#include "sim_graph_builder.hpp"

#include "pre/3_simulation_graph/elements/model_fmu.hpp"
#include "pre/3_simulation_graph/elements/model_connector.hpp"

#include <vector>

namespace ssp4sim::graph
{

    namespace
    {
        // Logical wire identity: source storage+index and target storage+index.
        // These fully determine the wire; type/size/delay/feedthrough/derivatives
        // are derived properties of the same wire and must not distinguish
        // duplicates.
        bool same_wire(const ConnectionInfo& a, const ConnectionInfo& b)
        {
            return a.source_storage == b.source_storage &&
                   a.source_index == b.source_index &&
                   a.target_storage == b.target_storage &&
                   a.target_index == b.target_index;
        }
    } // namespace

    void GraphBuilder::wire_connection(FmuModel* source_model, analysis::SspConnectorNode* conn_node)
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
            auto *peer_model_node = find_peer_model_node(peer_conn_node);

            // If the direct peer has no model, it may be a system boundary
            // connector. Trace through its downstream connections to find
            // the actual model connector.
            if (!peer_model_node || !peer_model_node->source)
            {
                analysis::SspConnectorNode* resolved_peer = nullptr;
                analysis::SspConnector* resolved_connector = nullptr;
                auto* traced = trace_boundary_connectors(peer_conn_node, resolved_peer, resolved_connector);
                if (traced)
                {
                    peer_model_node = traced;
                    peer_conn_node = resolved_peer;
                    peer_connector = resolved_connector;
                }
                else
                {
                    LOG_WARNING(log, "[{func}] Could not find peer model for connector {name}", __func__, peer_connector->name);
                    continue;
                }
            }

            auto tgt_it = models.find(peer_model_node->source->name);
            if (tgt_it == models.end())
            {
                LOG_WARNING(log, "[{func}] Peer model {name} not found in model map", __func__, peer_model_node->source->name);
                continue;
            }

            auto target_model = as_fmu(tgt_it->second.get());
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

            // Forward derivatives: enable when source provides derivatives and
            // target can interpolate them on real-typed signals.
            if (actual_source->maxOutputDerivativeOrder > 0 &&
                actual_target->canInterpolateInputs &&
                source_conn->type == types::DataType::real &&
                target_conn->type == types::DataType::real)
            {
                int order = static_cast<int>(actual_source->maxOutputDerivativeOrder);
                source_conn->forward_derivatives = true;
                source_conn->forward_derivatives_order = order;
                target_conn->forward_derivatives = true;
                target_conn->forward_derivatives_order = order;
                con_info.forward_derivatives = true;
                con_info.forward_derivatives_order = order;
            }

            LOG_TRACE_L1(log, "[{func}] Connection: {src}.{sc} -> {tgt}.{tc}, delay {delay}",
                         __func__,
                         actual_source->name, source_conn->name,
                         actual_target->name, target_conn->name,
                         con_info.delay);

            // Dedup guard: the same wire is legitimately reached
            // from BOTH endpoints
            bool already_wired = false;
            for (const ConnectionInfo &existing : actual_target->connections)
            {
                if (same_wire(existing, con_info))
                {
                    already_wired = true;
                    break;
                }
            }

            if (!already_wired)
            {
                actual_target->connections.push_back(std::move(con_info));
            }
        }
    }

    void GraphBuilder::wire_connections(analysis::AnalysisGraphData &graph_data)
    {
        LOG_DEBUG(log, "[{func}] - Wiring connections using connector->connection->connector graph", __func__);

        // Navigate the model→connector→connection→connector→model graph
        for (auto &model_node : graph_data.model_nodes)
        {
            auto *analysis_model = model_node->source;
            if (!analysis_model || !analysis_model->fmu)
                continue;

            auto src_it = models.find(analysis_model->name);
            if (src_it == models.end())
                continue;

            auto source_model = as_fmu(src_it->second.get());
            if (!source_model)
                continue;

            // For each connector on this model, find connections.
            // Output/parameter connectors are children, input connectors are parents.
            auto conn_children = model_node->template get_child_nodes<analysis::SspConnectorNode>();
            auto conn_parents = model_node->template get_parent_nodes<analysis::SspConnectorNode>();
            for (auto *conn_node : conn_children)
                wire_connection(source_model, conn_node);
            for (auto *conn_node : conn_parents)
                wire_connection(source_model, conn_node);
        }
    }

} // namespace ssp4sim::graph