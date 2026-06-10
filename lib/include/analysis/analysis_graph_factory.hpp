#pragma once

#include "analysis/components/analysis_system.hpp"

#include "utils/node.hpp"
#include "utils/tarjan.hpp"

#include <memory>
#include <string>
#include <vector>

namespace ssp4sim::analysis
{
    /// Transient node wrapping a connector or variable for graph traversal.
    /// Inherits from utils::graph::Node for Tarjan SCC compatibility.
    struct AnalysisNode : public utils::graph::Node
    {
        /// The source object this node wraps: either an AnalysisConnector
        /// or an AnalysisModelVariable (tagged via is_connector).
        bool is_connector = false;

        /// Reference to the original analysis data (non-owning).
        void *source = nullptr;

        AnalysisNode() = default;

        explicit AnalysisNode(const std::string &name_, bool is_connector_, void *source_ = nullptr)
            : Node(name_), is_connector(is_connector_), source(source_)
        {
        }
    };

    /// Factory that builds transient graph views from AnalysisSystem data.
    /// Created per call — does NOT own persistent state.
    class AnalysisGraphFactory
    {
    public:
        explicit AnalysisGraphFactory(const AnalysisSystem &system);

        /// Build transient connection graph and run Tarjan SCC to find algebraic loops.
        std::vector<std::vector<utils::graph::Node *>> find_algebraic_loops();

        /// Build the transient graph and return all nodes.
        std::vector<utils::graph::Node *> build_transient_graph();

    private:
        const AnalysisSystem &system_;
    };
} // namespace ssp4sim::analysis