#pragma once

#include <map>
#include <memory>
#include <string>

#include "pre/2_analysis/elements/ssp_node.hpp"
#include "shared_config.hpp"
#include "utils/primitives/node.hpp"

namespace ssp4sim::analysis {
    struct AnalysisGraphData;
}

namespace ssp4sim::graph {
    class Invocable;
}

namespace ssp4sim::pre {

class SimulationDebugWriter {
public:
    explicit SimulationDebugWriter(SharedConfig *config);

    /// Block 1: write analysis tree (tree_output.txt) and graph (graph_output.dot)
    void write_analysis_debug(
        analysis::SspSystemNode *system_tree,
        const analysis::AnalysisGraphData &graph_data);

    /// Block 2: write start values CSV (config->start_value_log_file)
    void write_start_values(utils::graph::Node *system_tree);

    /// Block 3: write model graph DOT (model_graph.dot)
    void write_model_graph(
        const std::map<std::string, std::unique_ptr<graph::Invocable>> &models);

private:
    SharedConfig *config_;
};

} // namespace ssp4sim::pre