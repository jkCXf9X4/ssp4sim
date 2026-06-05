#include "graph/analysis/analysis_graph_builder.hpp"

#include "FMI2_modelDescription_Ext.hpp"
#include "SSP1_SystemStructureDescription_Ext.hpp"
#include "SSP1_SystemStructureParameter_Ext.hpp"
#include "SSP_Ext.hpp"
#include "utils/time.hpp"

#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>

namespace ssp4sim::analysis::graph
{

    AnalysisGraphBuilder::AnalysisGraphBuilder(ssp4cpp::Ssp *ssp, handler::FmuHandler *fmu_handler)
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.graph.AnalysisGraphBuilder")),
          ssp(ssp),
          fmu_handler(fmu_handler)
    {
    }

    std::unique_ptr<AnalysisGraph> AnalysisGraphBuilder::build()
    {
        LOG_TRACE_L1(log, "[{func}] Building AnalysisGraph", __func__);
        auto models = create_models(*ssp, fmu_handler, log);
        auto connectors = create_connectors(*ssp, fmu_handler, log);
        auto connections = create_connections(*ssp, log);
        auto model_variables = create_model_variables(fmu_handler->fmu_ref_map, log);

        attach_connectors_to_models(connectors, models);
        wire_connections(connections, models, connectors);
        wire_internal_dependencies(model_variables, connectors);
        compute_feedthrough(connectors, models);

        LOG_TRACE_L1(log, "[{func}] exit", __func__);
        return make_unique<AnalysisGraph>(std::move(models), std::move(connectors), std::move(connections), std::move(model_variables));
    }

    void AnalysisGraphBuilder::attach_connectors_to_models(std::map<std::string, std::unique_ptr<AnalysisConnector>> &connectors, std::map<std::string, std::unique_ptr<AnalysisModel>> &models)
    {
        LOG_TRACE_L1(log, "[{func}] Attaching connectors to models", __func__);
        for (auto &[name, connector] : connectors)
        {
            if (!models.contains(connector->component_name))
            {
                LOG_ERROR(log, "[{func}] Attaching connector: Failed to attach connector to model, model {model} not found for connector {connector}", __func__, connector->component_name, connector->name);
                throw std::runtime_error("Failed to find model associated with connector ");
            }

            auto model = models[connector->component_name].get();
            if (model->connectors.count(connector->name))
            {
                LOG_ERROR(log, "[{func}] Naming conflict for connectors {connector}", __func__, connector->name);
                throw std::runtime_error("Naming conflict between connectors");
            }
            model->connectors[connector->name] = connector.get();
        }
    }

    void AnalysisGraphBuilder::wire_connections(std::map<std::string, std::unique_ptr<AnalysisConnection>> &connections, std::map<std::string, std::unique_ptr<AnalysisModel>> &models, std::map<std::string, std::unique_ptr<AnalysisConnector>> &connectors)
    {
        LOG_TRACE_L1(log, "[{func}] Creating connections between connectors", __func__);
        for (auto &[name, connection] : connections)
        {
            LOG_TRACE_L1(log, "[{func}] Connecting {connection}", __func__, connection->name);

            bool source_model_exist = models.contains(connection->source_component_name);
            bool target_model_exist = models.contains(connection->target_component_name);
            if (!source_model_exist || !target_model_exist)
            {
                LOG_ERROR(log, "[{func}] Creating connection: Failed to find source or target model. Exists s: {source_exists} t: {target_exists}\n {connection}",
                           __func__, source_model_exist, target_model_exist, connection->to_string());
                throw std::runtime_error("Failed to find connection model");
            }

            auto source_model = models[connection->source_component_name].get();
            auto target_model = models[connection->target_component_name].get();

            auto source_connector_name = connection->get_source_connector_name();
            auto target_connector_name = connection->get_target_connector_name();

            bool source_connector_exist = connectors.contains(source_connector_name);
            bool target_connector_exist = connectors.contains(target_connector_name);
            if (!source_connector_exist || !target_connector_exist)
            {
                LOG_ERROR(log, "[{func}] Creating connection: Failed to find source or target connector. Found source: {source} target: {target}\n {connection}",
                           __func__, source_connector_exist, target_connector_exist, connection->to_string());
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
    }

    // Created the internal graph
    void AnalysisGraphBuilder::wire_internal_dependencies(
        std::map<std::string, std::unique_ptr<AnalysisModelVariable>> &model_variables,
        std::map<std::string, std::unique_ptr<AnalysisConnector>> &connectors)
    {
        LOG_TRACE_L1(log, "[{func}] Connecting internal variable dependencies within FMUs", __func__);
        for (auto [fmu_name, fmu] : fmu_handler->fmu_ref_map)
        {
            LOG_DEBUG(log, "[{func}] Connecting internal dependencies, FMU:{fmu_name}", __func__, fmu_name);

            auto outputs = fmu->md->ModelStructure.Outputs;
            if (outputs.has_value())
            {
                std::vector<ext::fmi2::dependency::VariableDependencyCoupling> dependencies;
                try
                {
                    dependencies = ext::fmi2::dependency::get_dependencies_variables(
                        outputs.value().Unknowns,
                        fmu->md->ModelVariables,
                        ssp4cpp::fmi2::md::DependenciesKind::dependent);
                }
                catch (const std::exception &e)
                {
                    LOG_WARNING(log, "[{func}] Skipping dependencies for FMU {fmu_name}: {reason}", __func__, fmu_name, e.what());
                    continue;
                }

                for (auto &[source, target, kind] : dependencies)
                {
                    (void)kind;
                    auto source_id = AnalysisConnector::create_name(fmu_name, source->name);
                    auto target_id = AnalysisConnector::create_name(fmu_name, target->name);
                    ssp4sim::utils::graph::Node *source_node = nullptr;
                    ssp4sim::utils::graph::Node *target_node = nullptr;

                    if (connectors.contains(source_id))
                    {
                        source_node = connectors[source_id].get();
                        LOG_DEBUG(log, "[{func}] Source C {name}", __func__, connectors[source_id]->name);
                    }
                    else if (model_variables.contains(source_id))
                    {
                        source_node = model_variables[source_id].get();
                        LOG_DEBUG(log, "[{func}] Source V {name}", __func__, model_variables[source_id]->name);
                    }

                    if (connectors.contains(target_id))
                    {
                        target_node = connectors[target_id].get();
                        LOG_DEBUG(log, "[{func}] Target C {name}", __func__, connectors[target_id]->name);
                    }
                    else if (model_variables.contains(target_id))
                    {
                        target_node = model_variables[target_id].get();
                        LOG_DEBUG(log, "[{func}] Target V {name}", __func__, model_variables[target_id]->name);
                    }

                    if (source_node && target_node)
                    {
                        LOG_DEBUG(log, "[{func}] Connecting {source} -> {target}", __func__, source_node->name, target_node->name);
                        target_node->add_child(source_node);
                    }
                    else
                    {
                        LOG_WARNING(log,
                                    "[{func}] Failed to resolve nodes for dependency: {source_id} -> {target_id} in FMU {fmu_name}",
                                    __func__, source_id, target_id, fmu_name);
                    }
                }
            }
        }
    }

    // finds the feedthru points
    // should be moved to tarjans... 
    void AnalysisGraphBuilder::compute_feedthrough(
        std::map<std::string, std::unique_ptr<AnalysisConnector>> &connectors,
        std::map<std::string, std::unique_ptr<AnalysisModel>> &models)
    {
        LOG_TRACE_L1(log, "[{func}] Computing feedthrough from analysis graph", __func__);
        for (auto &[model_name, model] : models)
        {
            // Collect input and output connector Nodes for this FMU
            std::vector<ssp4sim::utils::graph::Node *> input_nodes;
            std::vector<AnalysisConnector *> output_connectors;
            for (auto &[name, conn] : model->connectors)
            {
                if (conn->causality == types::Causality::input)
                {
                    input_nodes.push_back(conn);
                }
                else if (conn->causality == types::Causality::output)
                {
                    output_connectors.push_back(conn);
                }
            }

            if (input_nodes.empty() || output_connectors.empty())
            {
                LOG_DEBUG(log, "[{func}] Model {model}: no inputs or no outputs, skipping", __func__, model_name);
                continue;
            }

            std::unordered_set<ssp4sim::utils::graph::Node *> input_set(input_nodes.begin(), input_nodes.end());

            for (auto *output : output_connectors)
            {
                // BFS following children edges to find transitive input dependency
                std::unordered_set<ssp4sim::utils::graph::Node *> visited;
                std::vector<ssp4sim::utils::graph::Node *> stack = {output};

                while (!stack.empty())
                {
                    auto *current = stack.back();
                    stack.pop_back();

                    if (input_set.count(current))
                    {
                        output->is_feedthrough = true;
                        LOG_DEBUG(log, "[{func}] Output {name} is feedthrough (input {input} reachable)",
                                  __func__, output->name, current->name);
                        break;
                    }

                    if (!visited.insert(current).second)
                        continue; // already visited

                    for (auto *child : current->children)
                    {
                        if (!visited.count(child))
                        {
                            stack.push_back(child);
                        }
                    }
                }

                if (!output->is_feedthrough)
                {
                    LOG_DEBUG(log, "[{func}] Output {name} is NOT feedthrough (no input reachable)",
                              __func__, output->name);
                }
            }
        }
    }

}
