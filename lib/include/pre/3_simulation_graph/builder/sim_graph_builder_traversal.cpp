#include "sim_graph_builder.hpp"

#include "pre/3_simulation/elements/model_fmu.hpp"

#include <vector>

namespace ssp4sim::graph
{

    static constexpr int kMaxBoundaryTraceDepth = 16;

    analysis::SspModelNode* GraphBuilder::find_peer_model_node(analysis::SspConnectorNode* conn_node)
    {
        for (auto *parent : conn_node->parents)
            if (auto *m = dynamic_cast<analysis::SspModelNode *>(parent))
                return m;
        for (auto *child : conn_node->children)
            if (auto *m = dynamic_cast<analysis::SspModelNode *>(child))
                return m;
        return nullptr;
    }

    analysis::SspModelNode* GraphBuilder::trace_boundary_connectors(
        analysis::SspConnectorNode* peer_conn_node,
        analysis::SspConnectorNode*& resolved_peer,
        analysis::SspConnector*& resolved_connector)
    {
        resolved_peer = peer_conn_node;
        for (int depth = 0; depth < kMaxBoundaryTraceDepth; ++depth)
        {
            auto next_conns = resolved_peer->template get_child_nodes<
                analysis::SspNode<analysis::ResolvedConnection>>();
            if (next_conns.empty())
                break;
            auto next_peers = next_conns[0]->template get_child_nodes<
                analysis::SspConnectorNode>();
            if (next_peers.empty())
                break;
            resolved_peer = next_peers[0];
            auto* peer_model_node = find_peer_model_node(resolved_peer);
            if (peer_model_node && peer_model_node->source)
            {
                resolved_connector = resolved_peer->source;
                return peer_model_node;
            }
        }
        return nullptr;
    }

} // namespace ssp4sim::graph