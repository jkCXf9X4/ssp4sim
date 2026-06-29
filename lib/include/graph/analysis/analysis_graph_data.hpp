#pragma once

#include "ssp4sim_definitions.hpp"

#include "analysis/components/analysis_system.hpp"
#include "analysis/components/analysis_connector.hpp"
#include "analysis/components/analysis_connection.hpp"
#include "analysis/components/analysis_model_variable.hpp"
#include "analysis/components/analysis_model.hpp"

#include <cstdint>
#include <string>

namespace ssp4sim::analysis
{
    /// Convenience aliases for the typed specializations.
    using ModelNode = AnalysisNode<AnalysisModel>;
    using ConnectorNode = AnalysisNode<AnalysisConnector>;
    using ConnectionNode = AnalysisNode<AnalysisConnection>;
    using VariableNode = AnalysisNode<AnalysisModelVariable>;
    using SystemNode = AnalysisNode<AnalysisSystem>;

    /// Complete output of AnalysisGraphFactory for injection into GraphBuilder.
    struct AnalysisGraphData
    {
        std::vector<std::unique_ptr<ModelNode>> model_nodes;
        std::vector<std::unique_ptr<ConnectorNode>> connector_nodes;
        std::vector<std::unique_ptr<ConnectionNode>> connection_nodes;
        std::vector<std::unique_ptr<VariableNode>> variable_nodes;
        std::vector<std::unique_ptr<SystemNode>> system_nodes;

        SystemNode *top_system;

        template <typename T>
        static void copyVector(const std::vector<std::unique_ptr<AnalysisNode<T>>> &src,
                               std::vector<std::unique_ptr<AnalysisNode<T>>> &dst)
        {
            dst.clear();
            dst.reserve(src.size());

            for (const auto &item : src)
            {
                dst.push_back(item.clone()));
            }
        }

        static void copy_data(AnalysisGraphData &source, AnalysisGraphData &target)
        {
            copyVector(source.model_nodes, target.model_nodes);
            copyVector(source.connector_nodes, target.connector_nodes);
            copyVector(source.connection_nodes, target.connection_nodes);
            copyVector(source.variable_nodes, target.variable_nodes);
            copyVector(source.system_nodes, target.system_nodes);
        }
    };

    template <typename T>
    T *create_node(T *content, std::vector<std::unique_ptr<T>> owner)
    {
        auto node = std::make_unique<T>(content->name, content);
        owner.push_back(std::move(node));
        return node.get();
    }

} // namespace ssp4sim::analysis