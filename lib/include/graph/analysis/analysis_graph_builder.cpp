#include "graph/analysis/analysis_graph_builder.hpp"

#include "SSP1_SystemStructureParameter_Ext.hpp"
#include "SSP_Ext.hpp"

#include "utils/time.hpp"

#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "SSP1_SystemStructureDescription_Ext.hpp"
#include "FMI2_modelDescription_Ext.hpp"

namespace ssp4sim::analysis::graph
{

    AnalysisGraphBuilder::AnalysisGraphBuilder(ssp4cpp::Ssp *ssp, handler::FmuHandler *fmu_handler)
        : ssp(ssp), fmu_handler(fmu_handler)
    {
    }

    std::map<std::string, std::unique_ptr<AnalysisModel>> AnalysisGraphBuilder::create_models(ssp4cpp::Ssp &ssp)
    {
        log(ext_trace)("[{}] init", __func__);
        std::map<std::string, std::unique_ptr<AnalysisModel>> models;

        for (auto &resource : ext::ssp::get_resources(*ssp.ssd))
        {
            auto ssp_resource_name = resource->name.value_or("null");

            auto fmu = fmu_handler->fmu_info_map[ssp_resource_name].get();
            auto m = std::make_unique<AnalysisModel>(ssp_resource_name, resource->source, fmu);

            if (fmu->model_description->CoSimulation)
            {
                auto co_sim = *fmu->model_description->CoSimulation;
                m->set_interpolation_data(co_sim.canInterpolateInputs.value_or(false), co_sim.maxOutputDerivativeOrder.value_or(0));
            }

            log(trace)("[{}] New Model: {}", __func__, m->name);
            models[m->name] = std::move(m);
        }
        log(ext_trace)("[{}] exit", __func__);
        return models;
    }

    std::map<std::string, std::unique_ptr<AnalysisConnector>> AnalysisGraphBuilder::create_connectors(ssp4cpp::Ssp &ssp)
    {
        log(ext_trace)("[{}] init", __func__);
        std::map<std::string, std::unique_ptr<AnalysisConnector>> items;
        if (ssp.ssd->System.Elements.has_value())
        {

            auto mapping_start_values = ssp4sim::ext::ssp1::ssv::get_start_value_mappings(ssp);

            for (auto &component : ssp.ssd->System.Elements.value().Components)
            {
                if (!component.name.has_value())
                {
                    log(error)("[{}] Component does not specify name attribute, Its optional but needed for this application {}", __func__);
                    throw std::runtime_error("Component without name");
                }

                auto component_name = component.name.value();

                if (!fmu_handler->fmu_info_map.contains(component_name))
                {
                    log(error)("[{}] Fmu not found, {}", __func__, component_name);
                    throw std::runtime_error("Fmu not found");
                }
                auto fmu = fmu_handler->fmu_info_map[component_name].get();

                auto md = fmu->model_description;

                auto variables = ext::fmi2::model_variables::get_variables(*md, {types::Causality::input, types::Causality::output, types::Causality::parameter});

                for (auto &var : variables)
                {
                    log(debug)("[{}] Creating Connector: {}.{}", __func__, component_name, var.name);
                    auto value_reference = var.valueReference.value();
                    log(ext_trace)("[{}] get_variable_type {}", __func__, value_reference);
                    auto type = ext::fmi2::model_variables::get_variable_type(var);

                    log(ext_trace)("[{}] Create AnalysisConnector", __func__);
                    auto c = std::make_unique<AnalysisConnector>(
                        component_name, var.name, value_reference, type);

                    c->causality = var.causality.value(); // it must have value to be selected in the list
                    auto system_name = component_name + "." + var.name;

                    auto start_value = ext::fmi2::model_variables::get_variable_start_value(var);
                    if (start_value)
                    {
                        log(debug)("[{}] Applying start value for {}", __func__, system_name);
                        c->initial_value = std::make_unique<ext::ssp1::ssv::StartValue>(var.name, type);
                        c->initial_value->store_value(start_value);
                    }

                    log(trace)("[{}] TODO: Internal SSP parameterset should overwrite the fmu", __func__);

                    if (mapping_start_values.contains(system_name))
                    {
                        log(debug)("[{}] Applying parameterset value to {}, {}", __func__, system_name, type.to_string());

                        const auto &mapped_start_value = mapping_start_values.at(system_name);
                        c->initial_value = std::make_unique<ext::ssp1::ssv::StartValue>(mapped_start_value);
                    }

                    if (c->initial_value)
                    {
                        log(debug)("[{}] Initial value {}", __func__, c->initial_value->to_string());
                    }

                    items[c->name] = std::move(c);
                }
            }
        }
        log(ext_trace)("[{}] exit, Total connectors created: {}", __func__, items.size());
        return items;
    }

    std::map<std::string, std::unique_ptr<AnalysisConnection>> AnalysisGraphBuilder::create_connections(ssp4cpp::Ssp &ssp)
    {
        log(ext_trace)("[{}] init", __func__);
        std::map<std::string, std::unique_ptr<AnalysisConnection>> items;
        if (ssp.ssd->System.Connections.has_value())
        {
            for (auto &connection : ssp.ssd->System.Connections.value().Connections)
            {
                auto c = std::make_unique<AnalysisConnection>(&connection);
                log(trace)("[{}] New Connection: {}", __func__, c->name);
                c->delay = utils::time::s_to_ns(connection.information_delay.value_or(0));
                items[c->name] = std::move(c);
            }
        }
        log(ext_trace)("[{}] exit, Total connections created: {}", __func__, items.size());
        return items;
    }

    std::map<std::string, std::unique_ptr<AnalysisModelVariable>> AnalysisGraphBuilder::create_model_variables(std::map<std::string, ssp4cpp::Fmu *> &fmu_map)
    {
        log(warning)("[{}] init, deprecated", __func__);
        std::map<std::string, std::unique_ptr<AnalysisModelVariable>> items;
        for (auto &[name, fmu] : fmu_map)
        {
            for (auto &variable : fmu->md->ModelVariables.ScalarVariable)
            {
                auto mv = std::make_unique<AnalysisModelVariable>(name, variable.name);
                log(trace)("[{}] New ModelVariable: {}", __func__, mv->name);
                items[mv->name] = std::move(mv);
            }
        }

        log(ext_trace)("[{}] exit, Total model variables created: {}", __func__, items.size());
        return items;
    }

    std::unique_ptr<AnalysisGraph> AnalysisGraphBuilder::build()
    {
        log(trace)("[{}] Building AnalysisGraph", __func__);
        auto models = create_models(*ssp);
        auto connectors = create_connectors(*ssp);
        auto connections = create_connections(*ssp);
        auto model_variables = create_model_variables(fmu_handler->fmu_ref_map);

        auto fmu_connections = ext::ssp1::elements::get_fmu_connections(*ssp->ssd);

        log(trace)("[{}] Connecting FMUs", __func__);
        for (auto &[source, target] : fmu_connections)
        {
            log(trace)("[{}] - Connecting: {} -> {}", __func__, source, target);
            models[source]->add_child(models[target].get());
        }

        log(trace)("[{}] Attaching connectors to models", __func__);
        for (auto &[name, connector] : connectors)
        {
            if (!models.contains(connector->component_name))
            {
                log(error)("Attaching connector: Failed to attach connector to model, model {} not found for connector {}", connector->component_name, connector->name);
                throw std::runtime_error("Failed to find model associated with connector ");
            }

            auto model = models[connector->component_name].get();
            if (model->connectors.count(connector->name))
            {
                log(error)("[{}] Naming conflict for connectors {}", __func__, connector->name);
                throw std::runtime_error("Naming conflict between connectors");
            }
            model->connectors[connector->name] = connector.get();
        }

        log(trace)("[{}] Creating connections between connectors", __func__);
        for (auto &[name, connection] : connections)
        {
            log(trace)("[{}] Connecting {}", __func__, connection->name);

            bool source_model_exist = models.contains(connection->source_component_name);
            bool target_model_exist = models.contains(connection->target_component_name);
            if (!source_model_exist or !target_model_exist)
            {
                log(error)("Creating connection: Failed to find source or target model. Exists s: {} t: {}\n {}",
                           source_model_exist, target_model_exist, connection->to_string());
                throw std::runtime_error("Failed to find connection model");
            }

            auto source_model = models[connection->source_component_name].get();
            auto target_model = models[connection->target_component_name].get();

            auto source_connector_name = connection->get_source_connector_name();
            auto target_connector_name = connection->get_target_connector_name();

            bool source_connector_exist = connectors.contains(source_connector_name);
            bool target_connector_exist = connectors.contains(target_connector_name);
            if (!source_connector_exist or !target_connector_exist)
            {
                log(error)("Creating connection: Failed to find source or target connector. Found source: {} target: {}\n {}",
                           source_connector_exist, target_connector_exist, connection->to_string());
                throw std::runtime_error("Failed to find connection connector");
            }
            auto source_connector = connectors[source_connector_name].get();
            auto target_connector = connectors[target_connector_name].get();

            source_connector->model = source_model;
            target_connector->model = target_model;

            connection->source_connector = source_connector;
            connection->source_model = source_model;
            connection->target_connector = target_connector;
            connection->target_model = target_model;

            source_connector->add_child(connection.get());
            connection->add_child(target_connector);

            // map if input outut derivatives should be forwarded

            if (source_model->maxOutputDerivativeOrder > 0 &&
                target_model->canInterpolateInputs &&
                source_connector->type == types::DataType::real &&
                target_connector->type == types::DataType::real)
            {
                source_connector->forward_derivatives = true;
                source_connector->forward_derivatives_order = source_model->maxOutputDerivativeOrder;
                target_connector->forward_derivatives = true;
                target_connector->forward_derivatives_order = source_model->maxOutputDerivativeOrder;
            }
        }

        // possible to add internal connections as well
        // see below

        log(ext_trace)("[{}] exit", __func__);
        return make_unique<AnalysisGraph>(std::move(models), std::move(connectors), std::move(connections));
    }

}

//     for (auto [fmu_name, fmu] : fmu_map)
//     {
//         log(debug)("[{}] Connecting internal dependencies, FMU:{}", __func__, fmu_name);

//         auto outputs = fmu->md.ModelStructure.Outputs;
//         if (outputs.has_value())
//         {

//             auto dependencies = ext::fmi2::dependency::get_dependencies_variables(
//                 outputs.value().Unknowns,
//                 fmu->md.ModelVariables,
//                 ssp4cpp::fmi2::md::DependenciesKind::dependent);

//             for (auto &[source, target, kind] : dependencies)
//             {
//                 auto source_id = Connector::create_name(fmu_name, source->name);
//                 auto target_id = Connector::create_name(fmu_name, target->name);
//                 SimNode *source_node;
//                 SimNode *target_node;

//                 // The connections can be from connectors or variables, maybe...
//                 if (connectors.contains(source_id))
//                 {
//                     source_node = connectors[source_id];
//                     log(debug)("[{}] Source C {}", __func__, connectors[source_id]->name);
//                 }
//                 else
//                 {
//                     source_node = variables[source_id];
//                     log(debug)("[{}] Source V {}", __func__, variables[source_id]->name);
//                 }

//                 if (connectors.contains(target_id))
//                 {
//                     target_node = connectors[target_id];
//                     log(debug)("[{}] Target C {}", __func__, connectors[target_id]->name);
//                 }
//                 else
//                 {
//                     target_node = variables[target_id];
//                     log(debug)("[{}] Target V {}", __func__, variables[target_id]->name);
//                 }
//                 log(debug)("[{}] Connecting {} -> {}", __func__, source_node->name, target_node->name);
//                 source_node->add_child(target_node);
//             }
//         }
//     }
