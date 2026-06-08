#include "graph/graph_builder.hpp"

#include "analysis/analysis_model.hpp"
#include "analysis/analysis_connector.hpp"
#include "analysis/analysis_connection.hpp"
#include "model/model_fmu.hpp"
#include "utils/map.hpp"

#include <cstdint>
#include <memory>
#include <set>
#include <utility>
#include <fstream>

namespace
{
    /// Find a connector by model name and (combined) connector name in the analysis system.
    const ssp4sim::analysis::AnalysisConnector *
    find_connector(const ssp4sim::analysis::AnalysisSystem &sys,
                   const std::string &model_name,
                   const std::string &connector_full_name)
    {
        for (auto *m : sys.get_all_models())
        {
            if (m->name != model_name)
                continue;
            for (auto &c : m->connectors)
            {
                if (c->name == connector_full_name)
                    return c.get();
            }
        }
        return nullptr;
    }
}

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

    void GraphBuilder::build()
    {
        LOG_DEBUG(log, "[{func}] init", __func__);

        create_fmu_models();
        create_data_storage_areas();
        wire_connections();
        derive_model_edges();

        LOG_DEBUG(log, "[{func}] - Allocate the input/output areas", __func__);
        for (auto &[ssp_resource_name, model] : models)
        {
            auto m = dynamic_cast<FmuModel *>(model.get());
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
                    info.initial_value = std::make_unique<ext::ssp1::ssv::StartValue>(*connector->initial_value);

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

    void GraphBuilder::wire_connections()
    {
        LOG_DEBUG(log, "[{func}] - Hand the information regarding the connections over to the model", __func__);

        for (auto *connection : analysis_system.get_all_connections())
        {
            // Skip boundary-crossing connections (not FMU-to-FMU)
            if (connection->is_boundary_crossing)
                continue;

            auto source_model = dynamic_cast<FmuModel *>(models[connection->source_model].get());
            auto target_model = dynamic_cast<FmuModel *>(models[connection->target_model].get());

            if (!source_model || !target_model)
            {
                LOG_WARNING(log, "[{func}] Skipping connection {src}.{sc} -> {tgt}.{tc}: model not found",
                            __func__, connection->source_model, connection->source_connector,
                            connection->target_model, connection->target_connector);
                continue;
            }

            auto source_connector_name = connection->source_model + "." + connection->source_connector;
            auto target_connector_name = connection->target_model + "." + connection->target_connector;

            auto &source_conn = source_model->outputs[source_connector_name];
            auto &target_conn = target_model->inputs[target_connector_name];

            ConnectionInfo con_info;
            con_info.type = source_conn.type;
            con_info.size = source_conn.size;

            con_info.source_storage = source_model->output_area.get();
            con_info.target_storage = target_model->input_area.get();
            con_info.source_index = source_conn.index;
            con_info.target_index = target_conn.index;

            con_info.forward_derivatives = source_conn.forward_derivatives;
            con_info.forward_derivatives_order = source_conn.forward_derivatives_order;

            con_info.delay = connection->delay;

            // Resolve feedthrough from the analysis connector
            auto *src_analysis_conn = find_connector(analysis_system,
                                                      connection->source_model,
                                                      source_connector_name);
            con_info.is_feedthrough = src_analysis_conn ? src_analysis_conn->is_feedthrough : false;
            if (connection->delay > 0)
            {
                con_info.is_feedthrough = false;
            }

            LOG_TRACE_L1(log, "[{func}] Connection: {src}.{sc} -> {tgt}.{tc}, delay {delay}",
                         __func__,
                         connection->source_model, connection->source_connector,
                         connection->target_model, connection->target_connector,
                         connection->delay);

            target_model->connections.push_back(std::move(con_info));
        }
    }

    void GraphBuilder::derive_model_edges()
    {
        LOG_DEBUG(log, "[{func}] Deriving model-to-model edges from connection graph", __func__);
        std::set<std::pair<std::string, std::string>> model_pairs;
        for (auto *connection : analysis_system.get_all_connections())
        {
            // Skip boundary-crossing connections
            if (connection->is_boundary_crossing)
                continue;

            model_pairs.insert({connection->source_model, connection->target_model});
        }

        LOG_DEBUG(log, "[{func}] Found {count} unique model pairs", __func__, model_pairs.size());
        for (auto &[source_name, target_name] : model_pairs)
        {
            LOG_TRACE_L1(log, "[{func}] - Model edge: {source} -> {target}", __func__, source_name, target_name);
            if (!models.contains(source_name))
            {
                LOG_WARNING(log, "[{func}] Source model {name} not found in model map, skipping", __func__, source_name);
                continue;
            }
            if (!models.contains(target_name))
            {
                LOG_WARNING(log, "[{func}] Target model {name} not found in model map, skipping", __func__, target_name);
                continue;
            }
            models[source_name]->add_child(models[target_name].get());
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