#include "sim_graph_builder.hpp"

#include "pre/3_simulation/elements/model_fmu.hpp"

#include <vector>

namespace ssp4sim::graph
{

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

            auto *source_fmu = as_fmu(src_it->second.get());
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
                    auto *peer_model_node = find_peer_model_node(peer_conn_node);

                    // Trace through system boundary connectors to find the
                    // actual model peer.
                    if (!peer_model_node)
                    {
                        analysis::SspConnectorNode* resolved_peer = nullptr;
                        analysis::SspConnector* resolved_connector = nullptr;
                        peer_model_node = trace_boundary_connectors(peer_conn_node, resolved_peer, resolved_connector);
                    }

                    if (!peer_model_node || !peer_model_node->source)
                        continue;

                    auto tgt_it = models.find(peer_model_node->source->name);
                    if (tgt_it == models.end())
                        continue;

                    auto *target_fmu = as_fmu(tgt_it->second.get());
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

} // namespace ssp4sim::graph