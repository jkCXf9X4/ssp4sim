#include "sim_graph_builder.hpp"

#include "pre/3_simulation/elements/model_fmu.hpp"
#include "pre/3_simulation/elements/model_connector.hpp"
#include "pre/1_ssp_parser/schema_extensions/FMI2_Enums_Ext.hpp"
#include "signal/recorder.hpp"
#include "utils/fmi/fmu_info.hpp"
#include "utils/primitives/map.hpp"

#include <cstdint>
#include <memory>
#include <set>
#include <utility>
#include <fstream>

namespace ssp4sim::graph
{
    GraphBuilder::GraphBuilder(bool record_inputs)
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.graph.GraphBuilder")),
          record_inputs(record_inputs)
    {
    }

    std::map<std::string, std::unique_ptr<Invocable>> GraphBuilder::build(analysis::AnalysisGraphData *graph_data)
    {
        LOG_DEBUG(log, "[{func}] init with pre-resolved graph data", __func__);

        create_fmu_models(*graph_data);
        create_data_storage_areas(*graph_data);
        wire_connections(*graph_data);
        derive_model_edges(*graph_data);

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
        }

        LOG_DEBUG(log, "[{func}] exit", __func__);
        return std::move(models);
    }

    void GraphBuilder::create_fmu_models(analysis::AnalysisGraphData &graph_data)
    {
        LOG_DEBUG(log, "[{func}] - Create the fmu models", __func__);
        for (auto &model_node : graph_data.model_nodes)
        {
            auto *analysis_model = model_node->source;
            if (!analysis_model)
                continue;

            // Skip models without FMU info
            if (!analysis_model->fmu)
            {
                LOG_DEBUG(log, "[{func}] -- Skipping model without FMU: {model}", __func__, analysis_model->name);
                continue;
            }

            // Create FmuInfo from the SspModel's FMU pointer and transfer ownership to FmuModel
            auto fmu_info = std::make_unique<handler::FmuInfo>(analysis_model->name, analysis_model->fmu.get());

            auto m = std::make_unique<FmuModel>(analysis_model->name, std::move(fmu_info), analysis_model->maxOutputDerivativeOrder);
            LOG_TRACE_L1(log, "[{func}] -- New Model: {model}", __func__, m->name);

            m->delay = analysis_model->delay;
            m->record_inputs = this->record_inputs;
            LOG_DEBUG(log, "[{func}] Model: {model}, delay {delay}", __func__, m->name, m->delay);

            models[analysis_model->name] = std::move(m);
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

            auto model = dynamic_cast<FmuModel *>(model_it->second.get());
            if (!model)
                continue;

            // Navigate connector nodes in both directions:
            // output/parameter connectors are children, input connectors are parents
            auto conn_children = model_node->template get_child_nodes<analysis::SspConnectorNode>();
            auto conn_parents = model_node->template get_parent_nodes<analysis::SspConnectorNode>();
            auto process_connector = [&](analysis::SspConnectorNode *conn_node) -> void
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
                    info.index = static_cast<uint32_t>(model->input_area->add(connector->name, connector->data_type, 0));
                    info.storage = model->input_area.get();
                    model->inputs[connector->name] = std::move(info);
                }
                else if (connector->causality == types::Causality::output)
                {
                    info.index = static_cast<uint32_t>(model->output_area->add(connector->name, connector->data_type, 0));
                    info.storage = model->output_area.get();
                    model->outputs[connector->name] = std::move(info);
                }
                else if (connector->causality == types::Causality::parameter)
                {
                    info.index = static_cast<uint32_t>(-1);
                    model->parameters[connector->name] = std::move(info);
                }
            };
            for (auto *conn_node : conn_children)
                process_connector(conn_node);
            for (auto *conn_node : conn_parents)
                process_connector(conn_node);
        }
    }

    void GraphBuilder::wire_connections(analysis::AnalysisGraphData &graph_data)
    {
        LOG_DEBUG(log, "[{func}] - Wiring connections using connector->connection->connector graph", __func__);

        // Helper: find the model that owns a connector, checking both parent
        // and child directions (output connectors are children, input connectors
        // are parents of the model in the graph).
        auto find_peer_model = [](analysis::SspConnectorNode *conn_node)
            -> analysis::SspModelNode *
        {
            for (auto *parent : conn_node->parents)
                if (auto *m = dynamic_cast<analysis::SspModelNode *>(parent))
                    return m;
            for (auto *child : conn_node->children)
                if (auto *m = dynamic_cast<analysis::SspModelNode *>(child))
                    return m;
            return nullptr;
        };

        // Process a single connector: trace its connection edges and wire up
        // source-output → target-input.
        auto process_connector = [&](analysis::SspConnectorNode *conn_node,
                                     FmuModel *source_model) -> void
        {
            auto *connector = conn_node->source;
            if (!connector)
                return;

            // Skip parameters — they are not wired
            if (connector->causality == types::Causality::parameter)
                return;

            // Find connection nodes attached to this connector
            for (auto *resolved_node : conn_node->get_child_nodes<analysis::SspNode<analysis::ResolvedConnection>>())
            {
                auto *resolved = resolved_node->source;
                if (!resolved)
                    continue;

                // Find the peer connector (the other end of the connection)
                auto peer_connectors = resolved_node->get_child_nodes<analysis::SspConnectorNode>();
                if (peer_connectors.empty())
                {
                    LOG_WARNING(log, "[{func}] Connection {name} has no peer connector", __func__, resolved_node->name);
                    continue;
                }

                auto *peer_conn_node = peer_connectors[0];
                auto *peer_connector = peer_conn_node->source;
                if (!peer_connector)
                    continue;

                // Find the peer model — check both parent and child directions
                auto *peer_model_node = find_peer_model(peer_conn_node);
                if (!peer_model_node || !peer_model_node->source)
                {
                    LOG_WARNING(log, "[{func}] Could not find peer model for connector {name}", __func__, peer_connector->name);
                    continue;
                }

                auto tgt_it = models.find(peer_model_node->source->name);
                if (tgt_it == models.end())
                {
                    LOG_WARNING(log, "[{func}] Peer model {name} not found in model map", __func__, peer_model_node->source->name);
                    continue;
                }

                auto target_model = dynamic_cast<FmuModel *>(tgt_it->second.get());
                if (!target_model)
                    continue;

                // Determine source and target based on causality
                ConnectorInfo *source_conn = nullptr;
                ConnectorInfo *target_conn = nullptr;

                if (connector->causality == types::Causality::output &&
                    peer_connector->causality == types::Causality::input)
                {
                    auto src_c = source_model->outputs.find(connector->name);
                    if (src_c != source_model->outputs.end())
                        source_conn = &src_c->second;

                    auto tgt_c = target_model->inputs.find(peer_connector->name);
                    if (tgt_c != target_model->inputs.end())
                        target_conn = &tgt_c->second;
                }
                else if (connector->causality == types::Causality::input &&
                         peer_connector->causality == types::Causality::output)
                {
                    // This connector is the input side, peer is the output
                    auto src_c = target_model->outputs.find(peer_connector->name);
                    if (src_c != target_model->outputs.end())
                        source_conn = &src_c->second;

                    auto tgt_c = source_model->inputs.find(connector->name);
                    if (tgt_c != source_model->inputs.end())
                        target_conn = &tgt_c->second;
                }

                if (!source_conn || !target_conn)
                {
                    LOG_WARNING(log, "[{func}] Could not resolve connector pair for {name}", __func__, resolved_node->name);
                    continue;
                }

                // Determine which model is source and which is target
                FmuModel *actual_source = nullptr;
                FmuModel *actual_target = nullptr;

                if (source_conn->storage == source_model->output_area.get())
                {
                    actual_source = source_model;
                    actual_target = target_model;
                }
                else
                {
                    actual_source = target_model;
                    actual_target = source_model;
                }

                ConnectionInfo con_info;
                con_info.type = source_conn->type;
                con_info.size = source_conn->size;

                con_info.source_storage = actual_source->output_area.get();
                con_info.target_storage = actual_target->input_area.get();
                con_info.source_index = source_conn->index;
                con_info.target_index = target_conn->index;

                con_info.delay = resolved->delay;
                con_info.is_feedthrough = (resolved->delay == 0);

                LOG_TRACE_L1(log, "[{func}] Connection: {src}.{sc} -> {tgt}.{tc}, delay {delay}",
                             __func__,
                             actual_source->name, source_conn->name,
                             actual_target->name, target_conn->name,
                             con_info.delay);

                actual_target->connections.push_back(std::move(con_info));
            }
        };

        // Navigate the model→connector→connection→connector→model graph
        for (auto &model_node : graph_data.model_nodes)
        {
            auto *analysis_model = model_node->source;
            if (!analysis_model || !analysis_model->fmu)
                continue;

            auto src_it = models.find(analysis_model->name);
            if (src_it == models.end())
                continue;

            auto source_model = dynamic_cast<FmuModel *>(src_it->second.get());
            if (!source_model)
                continue;

            // For each connector on this model, find connections.
            // Output/parameter connectors are children, input connectors are parents.
            auto conn_children = model_node->template get_child_nodes<analysis::SspConnectorNode>();
            auto conn_parents = model_node->template get_parent_nodes<analysis::SspConnectorNode>();
            for (auto *conn_node : conn_children)
                process_connector(conn_node, source_model);
            for (auto *conn_node : conn_parents)
                process_connector(conn_node, source_model);
        }
    }

    void GraphBuilder::derive_model_edges(analysis::AnalysisGraphData &graph_data)
    {
        LOG_DEBUG(log, "[{func}] Deriving model-to-model edges from graph", __func__);

        // Derive edges from the connection graph: if model A has a connection
        // to model B, then A is a dependency of B (A must execute before B)
        for (auto &model_node : graph_data.model_nodes)
        {
            auto *analysis_model = model_node->source;
            if (!analysis_model || !analysis_model->fmu)
                continue;

            auto src_it = models.find(analysis_model->name);
            if (src_it == models.end())
                continue;

            auto *source_fmu = dynamic_cast<FmuModel *>(src_it->second.get());
            if (!source_fmu)
                continue;

            // Find connections from this model's output connectors
            for (auto *conn_node : model_node->get_child_nodes<analysis::SspConnectorNode>())
            {
                auto *connector = conn_node->source;
                if (!connector || connector->causality != types::Causality::output)
                    continue;

                for (auto *resolved_node : conn_node->get_child_nodes<analysis::SspNode<analysis::ResolvedConnection>>())
                {
                    auto peer_connectors = resolved_node->get_child_nodes<analysis::SspConnectorNode>();
                    if (peer_connectors.empty())
                        continue;

                    auto *peer_conn_node = peer_connectors[0];
                    analysis::SspModelNode *peer_model_node = nullptr;
                    for (auto *parent : peer_conn_node->parents)
                    {
                        peer_model_node = dynamic_cast<analysis::SspModelNode *>(parent);
                        if (peer_model_node)
                            break;
                    }
                    if (!peer_model_node)
                    {
                        for (auto *child : peer_conn_node->children)
                        {
                            peer_model_node = dynamic_cast<analysis::SspModelNode *>(child);
                            if (peer_model_node)
                                break;
                        }
                    }

                    if (!peer_model_node || !peer_model_node->source)
                        continue;

                    auto tgt_it = models.find(peer_model_node->source->name);
                    if (tgt_it == models.end())
                        continue;

                    auto *target_fmu = dynamic_cast<FmuModel *>(tgt_it->second.get());
                    if (!target_fmu)
                        continue;

                    // source_fmu -> target_fmu (source is a dependency of target)
                    LOG_TRACE_L1(log, "[{func}] - Model edge: {source} -> {target}", __func__,
                                source_fmu->name, target_fmu->name);
                    source_fmu->add_child(target_fmu);
                }
            }
        }
    }

    void register_model_storages(
        const std::map<std::string, std::unique_ptr<Invocable>> &models,
        ssp4sim::signal::DataRecorder *recorder)
    {
        if (!recorder)
            return;

        for (auto &[name, model] : models)
        {
            auto m = dynamic_cast<FmuModel *>(model.get());
            if (!m)
                continue;

            if (m->record_inputs)
            {
                recorder->add_storage(m->input_area.get());
            }
            recorder->add_storage(m->output_area.get());
        }
    }

} // namespace ssp4sim::graph