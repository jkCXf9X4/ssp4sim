#include "sim_graph_builder.hpp"

#include "pre/3_simulation/elements/model_fmu.hpp"
#include "pre/3_simulation/elements/model_connector.hpp"
#include "pre/1_ssp_parser/schema_extensions/FMI2_Enums_Ext.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace ssp4sim::graph
{

    void GraphBuilder::register_connector_storage(FmuModel* model, analysis::SspConnectorNode* conn_node)
    {
        auto *connector = conn_node->source;
        if (!connector)
            return;

        ConnectorInfo info;
        info.type = connector->data_type;
        info.size = ssp4sim::ext::fmi2::enums::get_data_type_size(connector->data_type);
        info.name = connector->name;

        info.value_ref = connector->value_reference;
        info.fmu = model->fmu.get();

        // Store the initial value if available
        info.initial_value = std::make_unique<ext::ParameterValue>(connector->initial_value);

        if (connector->causality == types::Causality::input)
        {
            info.index = static_cast<uint32_t>(model->input_area->add_variable(connector->name, connector->data_type, model->maxOutputDerivativeOrder));
            info.storage = model->input_area.get();
            model->inputs[connector->name] = std::move(info);
        }
        else if (connector->causality == types::Causality::output)
        {
            info.index = static_cast<uint32_t>(model->output_area->add_variable(connector->name, connector->data_type, model->maxOutputDerivativeOrder));
            info.storage = model->output_area.get();
            model->outputs[connector->name] = std::move(info);
        }
        else if (connector->causality == types::Causality::parameter)
        {
            info.index = static_cast<uint32_t>(-1);
            model->parameters[connector->name] = std::move(info);
        }
    }

    void GraphBuilder::create_data_storage_areas(analysis::AnalysisGraphData &graph_data)
    {
        LOG_DEBUG(log, "[{func}] - Create the data storage areas within the model", __func__);

        for (auto &model_node : graph_data.model_nodes)
        {
            auto *analysis_model = model_node->source;
            if (!analysis_model || !analysis_model->fmu)
                continue;

            auto model_it = models.find(analysis_model->name);
            if (model_it == models.end())
                continue;

            auto model = as_fmu(model_it->second.get());
            if (!model)
                continue;

            // Navigate connector nodes in both directions:
            // output/parameter connectors are children, input connectors are parents
            auto conn_children = model_node->template get_child_nodes<analysis::SspConnectorNode>();
            auto conn_parents = model_node->template get_parent_nodes<analysis::SspConnectorNode>();
            for (auto *conn_node : conn_children)
                register_connector_storage(model, conn_node);
            for (auto *conn_node : conn_parents)
                register_connector_storage(model, conn_node);
        }
    }

} // namespace ssp4sim::graph