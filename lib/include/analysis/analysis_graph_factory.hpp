#pragma once

#include "analysis_graph_data.hpp"

#include "utils/node.hpp"
#include "utils/tarjan.hpp"

#include <memory>
#include <string>
#include <vector>

namespace ssp4sim::analysis
{
    class AnalysisGraphFactory
    {
    public:
        
        AnalysisGraphData full_graph;
        AnalysisGraphData model_graph;

        explicit AnalysisGraphFactory(AnalysisGraphData *tree);

        /// Build model-to-model graph (for simulation execution order).
        /// Nodes are AnalysisNode<AnalysisModel>, edges are model-to-model
        /// dependencies derived from resolved connections.
        std::vector<std::unique_ptr<ModelNode>> build_model_graph();

    private:
        AnalysisSystem *analysis_system;
    };
} // namespace ssp4sim::analysis