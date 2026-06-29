#include "graph/graph_builder.hpp"

#include "analysis/components/analysis_model.hpp"
#include "model/model_fmu.hpp"
#include "utils/map.hpp"

#include <cstdint>
#include <memory>
#include <set>
#include <utility>
#include <fstream>

namespace ssp4sim::graph
{
    GraphBuilder::GraphBuilder(const analysis::AnalysisSystem &analysis_system_,
                               ssp4sim::signal::DataRecorder *recorder,
                               ssp4sim::SharedConfig *config)
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.graph.GraphBuilder")),
          analysis_system(analysis_system_),
          recorder(recorder),
          config(config)
    {
    }

    void GraphBuilder::build_with_data(analysis::AnalysisGraphData &graph_data)
    {
        LOG_DEBUG(log, "[{func}] init with pre-resolved graph data", __func__);

        create_fmu_models();
        create_data_storage_areas();
        wire_connections(graph_data);
        derive_model_edges(graph_data);

        LOG_DEBUG(log, "[{func}] - Allocate the input/output areas", __func__);
        for (auto &[ssp_resource_name, model] : models)
        {
            auto m = dynamic_cast<FmuModel *>(model.get());
            if (!m)
            {
                LOG_WARNING(log, "[{func}] Skipping model '{name}' with null FmuModel pointer", __func__, ssp_resource_name);
                continue;
            }
            m->input_area->allocate();
            m->output_area->allocate();
            if (recorder)
            {
                if (m->record_inputs)
                {
                    recorder->add_storage(m->input_area.get());
                }
                recorder->add_storage(m->output_area.get());
            }
        }

        LOG_DEBUG(log, "[{func}] exit", __func__);
    }

    void GraphBuilder::create_fmu_models()
    {
        LOG_DEBUG(log, "[{func}] - Create the fmu models", __func__);
        for (auto *analysis_model : analysis_system.get_all_models())
        {
            // Skip models without FMU info (e.g., system containers)
            if (!analysis_model->fmu)
            {
                LOG_DEBUG(log, "[{func}] -- Skipping model without FMU: {model}", __func__, analysis_model->name);
                continue;
            }

            auto m = std::make_unique<FmuModel>(analysis_model->name, analysis_model->fmu, analysis_model->maxOutputDerivativeOrder);
            LOG_TRACE_L1(log, "[{func}] -- New Model: {model}", __func__, m->name);

            m->delay = analysis_model->delay;
            m->record_inputs = this->config->record_inputs;
            LOG_DEBUG(log, "[{func}] Model: {model}, delay {delay}", __func__, m->name, m->delay);

            models[analysis_model->name] = std::move(m);
        }
    }

    void GraphBuilder::create_data_storage_areas()
    {
        LOG_DEBUG(log, "[{func}] - Create the data storage areas within the model", __func__);
        auto start_value_log_file = std::ofstream(this->config->start_value_log_file, std::ios::out);
        for (auto *analysis_model : analysis_system.get_all_models())
        {
            if (!analysis_model->fmu)
                continue;

            auto model = dynamic_cast<FmuModel *>(models[analysis_model->name].get());
            if (!model)
            {
                LOG_WARNING(log, "[{func}] Model {name} not found in model map, skipping", __func__, analysis_model->name);
                continue;
            }

            for (auto &connector : analysis_model->connectors)
            {
                ConnectorInfo info;
                info.type = connector->data_type;
                info.size = connector->size;
                info.name = connector->name;

                info.forward_derivatives = connector->forward_derivatives;
                info.forward_derivatives_order = connector->forward_derivatives_order;

                info.value_ref = connector->value_reference;

                info.fmu = model->fmu;

                if (connector->initial_value)
                {
                    info.initial_value = std::make_unique<ext::ParameterValue>(*connector->initial_value);

                    auto value = ext::fmi2::enums::data_type_to_string(info.type, info.initial_value->raw_ptr());

                    LOG_TRACE_L1(log, "[{func}] -- Store start value for {} : {}", __func__, info.name, value);
                    start_value_log_file << connector->causality << ", " << connector->name << ", " << value << "\n";
                }

                if (connector->causality == types::Causality::input)
                {
                    info.index = static_cast<uint32_t>(model->input_area->add(connector->name, connector->data_type, connector->forward_derivatives_order));
                    info.storage = model->input_area.get();
                    model->inputs[connector->name] = std::move(info);
                }
                else if (connector->causality == types::Causality::output)
                {
                    info.index = static_cast<uint32_t>(model->output_area->add(connector->name, connector->data_type, connector->forward_derivatives_order));
                    info.storage = model->output_area.get();
                    model->outputs[connector->name] = std::move(info);
                }
                else if (connector->causality == types::Causality::parameter)
                {
                    info.index = static_cast<uint32_t>(-1);
                    model->parameters[connector->name] = std::move(info);
                }
            }
        }
    }

    void GraphBuilder::wire_connections(analysis::AnalysisGraphData &graph_data)
    {
        LOG_DEBUG(log, "[{func}] - Winding connections using pre-resolved graph data", __func__);

        // Use the pre-resolved connection list — no string matching needed
        for (auto &entry : graph_data.resolved_connections)
        {
            auto src_it = models.find(entry.source_model);
            auto tgt_it = models.find(entry.target_model);

            if (src_it == models.end() || tgt_it == models.end())
            {
                LOG_WARNING(log, "[{func}] Skipping pre-resolved connection {src}.{sc} -> {tgt}.{tc}: model not found",
                            __func__, entry.source_model, entry.source_connector,
                            entry.target_model, entry.target_connector);
                continue;
            }

            auto source_model = dynamic_cast<FmuModel *>(src_it->second.get());
            auto target_model = dynamic_cast<FmuModel *>(tgt_it->second.get());

            auto &source_conn = source_model->outputs[entry.source_connector];
            auto &target_conn = target_model->inputs[entry.target_connector];

            ConnectionInfo con_info;
            con_info.type = source_conn.type;
            con_info.size = source_conn.size;

            con_info.source_storage = source_model->output_area.get();
            con_info.target_storage = target_model->input_area.get();
            con_info.source_index = source_conn.index;
            con_info.target_index = target_conn.index;

            con_info.forward_derivatives = source_conn.forward_derivatives;
            con_info.forward_derivatives_order = source_conn.forward_derivatives_order;

            con_info.delay = entry.delay;

            // Resolve feedthrough from the pre-built connector node (no string matching)
            // Find the source connector node in the graph data
            bool feedthrough_found = false;
            for (auto &conn_node : graph_data.connector_nodes)
            {
                if (conn_node->source &&
                    conn_node->source->name == entry.source_connector &&
                    conn_node->model &&
                    conn_node->model->name == entry.source_model)
                {
                    con_info.is_feedthrough = conn_node->source->is_feedthrough;
                    feedthrough_found = true;
                    break;
                }
            }
            if (!feedthrough_found)
            {
                con_info.is_feedthrough = false;
            }
            if (entry.delay > 0)
            {
                con_info.is_feedthrough = false;
            }

            LOG_TRACE_L1(log, "[{func}] Connection: {src}.{sc} -> {tgt}.{tc}, delay {delay}",
                         __func__,
                         entry.source_model, entry.source_connector,
                         entry.target_model, entry.target_connector,
                         entry.delay);

            target_model->connections.push_back(std::move(con_info));
        }
    }

    void GraphBuilder::derive_model_edges(analysis::AnalysisGraphData &graph_data)
    {
        LOG_DEBUG(log, "[{func}] Deriving model-to-model edges from pre-built model graph", __func__);

        // Use the pre-built model graph edges — no string matching needed
        for (auto &node : graph_data.model_nodes)
        {
            if (!node->source)
                continue;

            auto src_it = models.find(node->source->name);
            if (src_it == models.end())
            {
                LOG_WARNING(log, "[{func}] Source model {name} not found in model map, skipping", __func__, node->source->name);
                continue;
            }

            for (auto *child : node->children)
            {
                auto *child_model_node = dynamic_cast<analysis::ModelNode *>(child);
                if (!child_model_node || !child_model_node->source)
                    continue;

                auto tgt_it = models.find(child_model_node->source->name);
                if (tgt_it == models.end())
                {
                    LOG_WARNING(log, "[{func}] Target model {name} not found in model map, skipping", __func__, child_model_node->source->name);
                    continue;
                }

                LOG_TRACE_L1(log, "[{func}] - Model edge: {source} -> {target}", __func__,
                            src_it->first, tgt_it->first);
                src_it->second->add_child(tgt_it->second.get());
            }
        }
    }

    std::unique_ptr<Graph> GraphBuilder::get_graph()
    {
        return std::make_unique<Graph>(ssp4sim::utils::map_ns::map_unique_to_ref(models), recorder);
    }

    std::map<std::string, std::unique_ptr<Invocable>> GraphBuilder::get_models()
    {
        return std::move(models);
    }

}