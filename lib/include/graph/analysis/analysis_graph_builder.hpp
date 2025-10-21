#pragma once

#include "utils/map.hpp"
#include "utils/vector.hpp"

#include "SSP_Ext.hpp"
#include "FMI2_modelDescription_Ext.hpp"
#include "FMI2_Enums_Ext.hpp"

#include "fmu_handler.hpp"

#include "data_type.hpp"

#include "analysis_connection.hpp"
#include "analysis_model.hpp"
#include "analysis_connector.hpp"
#include "analysis_internal.hpp"

#include "analysis_graph.hpp"

#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <list>

namespace ssp4sim::sim::analysis::graph
{
    using namespace std;

    class AnalysisGraphBuilder
    {
    public:
        static inline auto log = Logger("ssp4sim.graph.AnalysisGraphBuilder", LogLevel::info);

        ssp4sim::Ssp *ssp;
        handler::FmuHandler *fmu_handler;

        AnalysisGraphBuilder(ssp4sim::Ssp *ssp, handler::FmuHandler *fmu_handler)
        {
            this->ssp = ssp;
            this->fmu_handler = fmu_handler;
        }

        map<string, std::unique_ptr<AnalysisModel>> create_models(ssp4sim::Ssp &ssp)
        {
            log(ext_trace)("[{}] init", __func__);
            map<string, std::unique_ptr<AnalysisModel>> models;
            for (auto &[ssp_resource_name, local_resource_name] : ssp::ext::get_resource_map(ssp))
            {
                auto fmu = fmu_handler->fmu_info_map[ssp_resource_name].get();
                auto m = make_unique<AnalysisModel>(ssp_resource_name, local_resource_name, fmu);
                if (fmu->model_description->CoSimulation)
                {
                    auto co_sim = *fmu->model_description->CoSimulation;
                    m->set_interpolation_data(co_sim.canInterpolateInputs.value_or(false), co_sim.maxOutputDerivativeOrder.value_or(0));
                }

                log(trace)("[{}] New Model: {}", __func__, m->name);
                models[m->name] = std::move(m);
            }
            log(ext_trace)("[{}] exit", __func__);
            return std::move(models);
        }

        map<string, std::unique_ptr<AnalysisConnector>> create_connectors(ssp4sim::Ssp &ssp)
        {
            log(ext_trace)("[{}] init", __func__);
            map<string, std::unique_ptr<AnalysisConnector>> items;
            if (ssp.ssd->System.Elements.has_value())
            {
                auto connectors = ssp1::ext::elements::get_connectors(
                    ssp.ssd->System.Elements.value(),
                    {ssp4sim::fmi2::md::Causality::input, ssp4sim::fmi2::md::Causality::output, ssp4sim::fmi2::md::Causality::parameter});

                auto start_values = ssp1::ext::ssv::get_start_value_mappings(ssp.dir, *ssp.ssd);

                for (auto &[index, connector, component] : connectors)
                {
                    auto component_name = component->name.value();
                    auto connector_name = connector->name;
                    log(trace)("[{}] Creating Connector: {} - {}", __func__, component_name, connector_name);

                    // using namespace handler;
                    auto fmu = fmu_handler->fmu_info_map[component_name].get();

                    auto md = fmu->model_description;

                    log(ext_trace)("[{}] get_variable_by_name", __func__);
                    auto var = fmi2::ext::model_variables::get_variable_by_name(md->ModelVariables, connector_name);

                    log(ext_trace)("[{}] Name {}", __func__, var->name);
                    if (var)
                    {
                        auto value_reference = var->valueReference.value();
                        log(ext_trace)("[{}] get_variable_type {}", __func__, value_reference);
                        auto type = fmi2::ext::model_variables::get_variable_type(*var);

                        log(ext_trace)("[{}] Create AnalysisConnector", __func__);
                        auto c = std::make_unique<AnalysisConnector>(
                            component_name, connector_name, value_reference, type);

                        c->causality = connector->kind;

                        auto start_value = fmi2::ext::model_variables::get_variable_start_value(*var);
                        if (start_value)
                        {
                            log(debug)("[{}] Storing start value for {}.{}, {}", __func__, component_name, var->name, type.to_string());
                            c->initial_value = std::make_unique<ssp1::ext::ssv::StartValue>(var->name, type);
                            c->initial_value->store_value(start_value);
                            log(debug)("[{}] - Value {}", __func__, fmi2::ext::enums::data_type_to_string(type, c->initial_value->value.get()));
                        }

                        // parameter set might overwrite initial value
                        auto parameter_name = std::format("{}.{}", component_name, var->name);
                        log(ext_trace)("Parameter name {}", parameter_name);
                        if (start_values.contains(parameter_name))
                        {
                            log(debug)("[{}] Applying parameter for {} - {}", __func__, parameter_name, type.to_string());
                            const auto &start_value = start_values.at(parameter_name);
                            c->initial_value = std::make_unique<ssp1::ext::ssv::StartValue>(start_value);
                        }

                        items[c->name] = std::move(c);
                    }
                }
            }
            log(ext_trace)("[{}] exit, Total connectors created: {}", __func__, items.size());
            return std::move(items);
        }

        map<string, std::unique_ptr<AnalysisConnection>> create_connections(ssp4sim::Ssp &ssp)
        {
            log(ext_trace)("[{}] init", __func__);
            map<string, std::unique_ptr<AnalysisConnection>> items;
            if (ssp.ssd->System.Connections.has_value())
            {
                for (auto &connection : ssp.ssd->System.Connections.value().Connections)
                {
                    auto c = make_unique<AnalysisConnection>(&connection);
                    log(trace)("[{}] New Connection: {}", __func__, c->name);
                    items[c->name] = std::move(c);
                }
            }
            log(ext_trace)("[{}] exit, Total connections created: {}", __func__, items.size());
            return std::move(items);
        }

        map<string, std::unique_ptr<AnalysisModelVariable>> create_model_variables(map<string, ssp4sim::Fmu *> &fmu_map)
        {
            log(warning)("[{}] init, deprecated", __func__);
            map<string, std::unique_ptr<AnalysisModelVariable>> items;
            for (auto [name, fmu] : fmu_map)
            {
                for (auto &variable : fmu->md->ModelVariables.ScalarVariable)
                {
                    auto mv = make_unique<AnalysisModelVariable>(name, variable.name);
                    log(trace)("[{}] New ModelVariable: {}", __func__, mv->name);
                    items[mv->name] = std::move(mv);
                }
            }

            log(ext_trace)("[{}] exit, Total model variables created: {}", __func__, items.size());
            return std::move(items);
        }

        std::unique_ptr<AnalysisGraph> build()
        {
            log(trace)("[{}] Building AnalysisGraph", __func__);
            auto models = create_models(*ssp);
            auto connectors = create_connectors(*ssp);
            auto connections = create_connections(*ssp);
            auto model_variables = create_model_variables(fmu_handler->fmu_ref_map);

            auto fmu_connections = ssp1::ext::elements::get_fmu_connections(*ssp->ssd);

            log(trace)("[{}] Connecting FMUs", __func__);
            for (auto &[source, target] : fmu_connections)
            {
                log(trace)("[{}] - Connecting: {} -> {}", __func__, source, target);
                models[source]->add_child(models[target].get());
            }

            log(trace)("[{}] Attaching connectors to models", __func__);
            for (auto &[name, connector] : connectors)
            {
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

                auto source_model = models[connection->source_component_name].get();
                auto target_model = models[connection->target_component_name].get();

                auto source_connector_name = connection->get_source_connector_name();
                auto target_connector_name = connection->get_target_connector_name();

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
                    source_connector->type == utils::DataType::real &&
                    target_connector->type == utils::DataType::real)
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
    };

}

//     for (auto [fmu_name, fmu] : fmu_map)
//     {
//         log(debug)("[{}] Connecting internal dependencies, FMU:{}", __func__, fmu_name);

//         auto outputs = fmu->md.ModelStructure.Outputs;
//         if (outputs.has_value())
//         {

//             auto dependencies = ssp4sim::fmi2::ext::dependency::get_dependencies_variables(
//                 outputs.value().Unknowns,
//                 fmu->md.ModelVariables,
//                 ssp4sim::fmi2::md::DependenciesKind::dependent);

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
