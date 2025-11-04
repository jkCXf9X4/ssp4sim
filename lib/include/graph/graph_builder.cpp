#include "graph/graph_builder.hpp"

#include "graph/analysis/analysis_graph.hpp"
#include "graph/analysis/analysis_model.hpp"
#include "graph/analysis/analysis_connection.hpp"
#include "model/model_fmu.hpp"
#include "utils/data_ring_storage.hpp"
#include "utils/map.hpp"

#include <memory>

namespace ssp4sim::graph
{
    GraphBuilder::GraphBuilder(AnalysisGraph *ag, ssp4sim::utils::DataRecorder *recorder)
        : analysis_graph(ag), recorder(recorder)
    {
    }

    void GraphBuilder::build()
    {
        log(trace)("[{}] init", __func__);

        log(trace)("[{}] - Create the fmu models", __func__);
        for (auto &[ssp_resource_name, analysis_model] : analysis_graph->models)
        {
            auto m = std::make_unique<FmuModel>(ssp_resource_name, analysis_model->fmu);
            m->recorder = recorder;
            log(ext_trace)("[{}] -- New Model: {}", __func__, m->name);

            m->delay = analysis_model->delay;
            m->is_delay_modeled = analysis_model->is_delay_modeled;
            log(debug)("Model: {}, delay {}, is_delay_modeled: {}", m->name, m->delay, m->is_delay_modeled);

            models[analysis_model->name] = std::move(m);
        }

        log(trace)("[{}] - Create the data storage areas within the model", __func__);
        for (auto &[_, analysis_model] : analysis_graph->models)
        {
            auto model = static_cast<FmuModel *>(models[analysis_model->name].get());
            for (auto &[name, connector] : analysis_model->connectors)
            {
                int index = -1;
                if (connector->causality == types::Causality::input)
                    index = model->input_area->add(name, connector->type, connector->forward_derivatives_order);
                else if (connector->causality == types::Causality::output)
                    index = model->output_area->add(name, connector->type, connector->forward_derivatives_order);

                ConnectorInfo info;
                info.type = connector->type;
                info.size = connector->size;
                info.name = name;

                info.forward_derivatives = connector->forward_derivatives;
                info.forward_derivatives_order = connector->forward_derivatives_order;

                info.index = static_cast<uint32_t>(index);
                info.value_ref = connector->value_reference;

                info.fmu = model->fmu;

                if (connector->initial_value)
                {
                    info.initial_value = connector->initial_value->get_value();

                    log(debug)("[{}] -- Store start value for {} : {}", __func__, info.name, ssp4sim::ext::fmi2::enums::data_type_to_string(info.type, static_cast<void *>(info.initial_value.get())));
                }

                if (connector->causality == types::Causality::input)
                {
                    info.storage = model->input_area.get();
                    model->inputs[name] = std::move(info);
                }
                else if (connector->causality == types::Causality::output)
                {
                    info.storage = model->output_area.get();
                    model->outputs[name] = std::move(info);
                }
                else if (connector->causality == types::Causality::parameter)
                {
                    model->parameters[name] = std::move(info);
                }
            }
        }

        log(trace)("[{}] - Hand the information regarding the connections over to the model", __func__);
        for (auto &[_, connection] : analysis_graph->connections)
        {
            auto source_model = static_cast<FmuModel *>(models[connection->source_model->name].get());
            auto target_model = static_cast<FmuModel *>(models[connection->target_model->name].get());

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
            log(debug)("Connection: {}, delay {}", connection->name, connection->delay);

            target_model->connections.push_back(std::move(con_info));
        }

        log(trace)("[{}] - Allocate the input/output areas", __func__);
        for (auto &[ssp_resource_name, model] : models)
        {
            auto m = static_cast<FmuModel *>(model.get());
            m->input_area->allocate();
            m->output_area->allocate();
            recorder->add_storage(m->input_area->data.get());
            recorder->add_storage(m->output_area->data.get());
        }

        log(trace)("[{}] - Create connections between models", __func__);
        for (auto &[_, analysis_model] : analysis_graph->models)
        {
            for (auto &child : analysis_model->children)
            {
                models[analysis_model->name]->add_child(models[child->name].get());
            }
        }

        log(ext_trace)("[{}] exit", __func__);
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
