#pragma once

#include "../ssp_graph_data.hpp"

#include <memory>
#include <string>
#include <vector>

namespace ssp4sim::analysis
{

    class SspGraphBuilder
    {
    public:
        SspGraphBuilder() = default;

        /// Build the resolved connection graph from the tree.
        /// Produces AnalysisGraphData with model→connector→connection→connector→model
        /// graph structure. All nodes are owned by the returned AnalysisGraphData.
        AnalysisGraphData build(const SspSystemNode *tree);
    };

} // namespace ssp4sim::analysis