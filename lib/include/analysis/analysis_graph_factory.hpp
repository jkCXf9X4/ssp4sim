#pragma once

#include "analysis/components/analysis_system.hpp"
#include "analysis/components/analysis_connector.hpp"
#include "analysis/components/analysis_connection.hpp"
#include "analysis/components/analysis_model.hpp"

#include "utils/node.hpp"
#include "utils/tarjan.hpp"

#include <memory>
#include <string>
#include <vector>

namespace ssp4sim::analysis
{
    /// Typed transient graph node wrapping an analysis data object.
    /// Inherits from utils::graph::Node for Tarjan SCC compatibility.
    /// T is the source type (AnalysisModel, AnalysisConnector, AnalysisConnection,
    /// or AnalysisModelVariable).
    template <typename T>
    struct AnalysisNode : public utils::graph::Node
    {
        /// The source object this node wraps (non-owning).
        T *source = nullptr;

        /// Owning model for connector/variable nodes (null for model nodes).
        /// Enables GraphBuilder to resolve connector→model without string matching.
        AnalysisModel *model = nullptr;

        AnalysisNode() = default;

        explicit AnalysisNode(const std::string &name_, bool /*dummy*/,
                              T *source_ = nullptr,
                              AnalysisModel *model_ = nullptr)
            : Node(name_), source(source_), model(model_)
        {
        }
    };

    /// Convenience aliases for the typed specializations.
    using ModelNode = AnalysisNode<AnalysisModel>;
    using ConnectorNode = AnalysisNode<AnalysisConnector>;
    using ConnectionNode = AnalysisNode<AnalysisConnection>;
    using VariableNode = AnalysisNode<AnalysisModelVariable>;

    /// Pre-resolved connection entry for GraphBuilder (no string matching needed).
    struct ResolvedConnectionEntry
    {
        std::string source_model;
        std::string source_connector;
        std::string target_model;
        std::string target_connector;
        uint64_t delay;
    };

    /// Complete output of AnalysisGraphFactory for injection into GraphBuilder.
    struct AnalysisGraphData
    {
        /// Model-to-model graph: ModelNode nodes, edges = execution-order dependencies.
        std::vector<std::unique_ptr<ModelNode>> model_nodes;

        /// Connector connection graph: ConnectorNode nodes, edges = connections.
        std::vector<std::unique_ptr<ConnectorNode>> connector_nodes;

        /// Pre-resolved connections for wire_connections (eliminates all string matching).
        std::vector<ResolvedConnectionEntry> resolved_connections;
    };

    /// Factory that builds typed graphs from AnalysisSystem data.
    /// Created per call — does NOT own persistent state.
    class AnalysisGraphFactory
    {
    public:
        explicit AnalysisGraphFactory(const AnalysisSystem &system);

        /// Build both graphs and return them as a single data package.
        AnalysisGraphData build_all();

        /// Build model-to-model graph (for simulation execution order).
        /// Nodes are AnalysisNode<AnalysisModel>, edges are model-to-model
        /// dependencies derived from resolved connections.
        std::vector<std::unique_ptr<ModelNode>> build_model_graph();

        /// Build connector connection graph (for algebraic loop detection).
        /// Nodes are AnalysisNode<AnalysisConnector>, edges represent connections.
        /// The returned connector nodes already have model pointers set.
        std::vector<std::unique_ptr<ConnectorNode>> build_connector_graph();

        /// Build transient connection graph and run Tarjan SCC to find algebraic loops.
        std::vector<std::vector<utils::graph::Node *>> find_algebraic_loops();

    private:
        const AnalysisSystem &system_;
    };
} // namespace ssp4sim::analysis