#pragma once

#include <vector>

namespace ssp4sim::utils::graph
{
    class Node;
}

namespace ssp4sim::analysis
{
    /// Lightweight transient view of graph nodes for SCC/traversal.
    struct AnalysisGraphView
    {
        std::vector<utils::graph::Node *> nodes;

        static AnalysisGraphView from_nodes(std::vector<utils::graph::Node *> node_list)
        {
            return AnalysisGraphView{std::move(node_list)};
        }
    };
}