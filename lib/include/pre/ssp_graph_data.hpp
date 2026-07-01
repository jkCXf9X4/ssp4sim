#pragma once

#include "pre/2_analysis/elements/ssp_node.hpp"

#include <memory>
#include <string>
#include <vector>

namespace ssp4sim::analysis
{

    /// Resolved connection between two connectors.
    /// Extends SspItem so it can be wrapped by SspNode<T>.
    struct ResolvedConnection : public SspItem
    {
        uint64_t delay = 0;

        ResolvedConnection()
        {
            type = SspItemType::Connection;
        }
    };

    /// Analysis graph data produced by SspGraphBuilder and consumed by
    /// graph::GraphBuilder. All nodes are owned by typed vectors; graph edges
    /// between nodes are non-owning raw pointers (via add_child()).
    ///
    /// Declaration order matters: owning vectors must be declared BEFORE the
    /// vectors whose nodes point into them, because C++ destroys members in
    /// reverse declaration order.
    struct AnalysisGraphData
    {
        /// Owns the underlying ResolvedConnection objects that the
        /// connection_nodes' source pointers reference.
        std::vector<std::unique_ptr<ResolvedConnection>> connection_sources;

        std::vector<std::unique_ptr<SspModelNode>> model_nodes;
        std::vector<std::unique_ptr<SspConnectorNode>> connector_nodes;
        std::vector<std::unique_ptr<SspNode<ResolvedConnection>>> connection_nodes;
    };

} // namespace ssp4sim::analysis