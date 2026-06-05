#include "graph/graph_builder.hpp"

#include "graph/analysis/analysis_graph.hpp"
#include "model/model_fmu.hpp"
#include "utils/map.hpp"

#include <cstdint>
#include <memory>
#include <set>
#include <utility>
#include <fstream>

namespace ssp4sim::graph
{
    GraphBuilder::GraphBuilder(AnalysisGraph *ag, ssp4sim::signal::DataRecorder *recorder, ssp4sim::SharedConfig *config)
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.graph.GraphBuilder")),
          analysis_graph(ag),
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
                // register input storage if record_inputs config is enabled
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
        for (auto &[ssp_resource_name, analysis_model] : analysis_graph->models)
        {
            auto m = std::make_unique<FmuModel>(ssp_resource_name, analysis_model->fmu, analysis_model->maxOutputDerivativeOrder);
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
        for (auto &[_, analysis_model] : analysis_graph->models)
        {
            auto model = dynamic_cast<FmuModel *>(models[analysis_model->name].get());
            for (auto &[name, connector] : analysis_model->connectors)
            {
                ConnectorInfo info;
                info.type = connector->type;
                info.size = connector->size;
                info.name = name;

                info.forward_derivatives = connector->forward_derivatives;
                info.forward_derivatives_order = connector->forward_derivatives_order;

                info.value_ref = connector->value_reference;

                info.fmu = model->fmu;

                if (connector->initial_value)
                {
                    info.initial_value = std::make_unique<ext::ssp1::ssv::StartValue>(*connector->initial_value);

                    auto value = ssp4sim::ext::fmi2::enums::data_type_to_string(info.type, info.initial_value->raw_ptr());

                    LOG_TRACE_L1(log, "[{func}] -- Store start value for {} : {}", __func__, info.name, value);
                    start_value_log_file << connector->causality << ", " << name << ", " <<  value << "\n";
                }

                if (connector->causality == types::Causality::input)
                {
                    info.index = static_cast<uint32_t>(model->input_area->add(name, connector->type, connector->forward_derivatives_order));
                    info.storage = model->input_area.get();
                    model->inputs[name] = std::move(info);
                }
                else if (connector->causality == types::Causality::output)
                {
                    info.index = static_cast<uint32_t>(model->output_area->add(name, connector->type, connector->forward_derivatives_order));
                    info.storage = model->output_area.get();
                    model->outputs[name] = std::move(info);
                }
                else if (connector->causality == types::Causality::parameter)
                {
                    info.index = static_cast<uint32_t>(-1);
                    model->parameters[name] = std::move(info);
                }
            }
        }
    }

    void GraphBuilder::wire_connections()
    {
        LOG_DEBUG(log, "[{func}] - Hand the information regarding the connections over to the model", __func__);

        for (auto &[_, connection] : analysis_graph->connections)
        {
            auto source_model = dynamic_cast<FmuModel *>(models[connection->source_model->name].get());
            auto target_model = dynamic_cast<FmuModel *>(models[connection->target_model->name].get());

            auto &source_connector = source_model->outputs[connection->get_source_connector_name()];
            auto &target_connector = target_model->inputs[connection->get_target_connector_name()];

            ConnectionInfo con_info;
            con_info.type = source_connector.type;
            con_info.size = source_connector.size;

            con_info.source_storage = source_model->output_area.get();
            con_info.target_storage = target_model->input_area.get();
            con_info.source_index = source_connector.index;
            con_info.target_index = target_connector.index;

            con_info.forward_derivatives = source_connector.forward_derivatives;
            con_info.forward_derivatives_order = source_connector.forward_derivatives_order;

            con_info.delay = connection->delay;

            con_info.is_feedthrough = connection->source_connector->is_feedthrough;
            if (connection->delay > 0)
            {
                con_info.is_feedthrough = false;
            }

            LOG_TRACE_L1(log, "[{func}] Connection: {name}, delay {delay}", __func__, connection->name, connection->delay);

            target_model->connections.push_back(std::move(con_info));
        }
    }

    void GraphBuilder::derive_model_edges()
    {
        LOG_DEBUG(log, "[{func}] Deriving model-to-model edges from connection graph", __func__);
        // Collect unique (source_model, target_model) pairs from analysis connections
        std::set<std::pair<std::string, std::string>> model_pairs;
        for (auto &[_, connection] : analysis_graph->connections)
        {
            auto const &src = connection->source_model->name;
            auto const &tgt = connection->target_model->name;
            model_pairs.insert({src, tgt});
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
