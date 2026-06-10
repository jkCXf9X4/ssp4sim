#pragma once

#include "analysis/analysis_graph_view.hpp"

#include "handler/fmu_handler.hpp"

#include "ssp4cpp/schema/ssp1/SSP1_SystemStructureDescription.hpp"

#include "utils/node.hpp"
#include "utils/tarjan.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ssp4sim::analysis
{
    // Forward declarations for data objects (created in Commit 2)
    class AnalysisModel;
    class AnalysisConnector;
    class AnalysisConnection;
    class AnalysisModelVariable;

    class AnalysisSystem
    {
    public:
        std::string name;

        std::vector<std::unique_ptr<AnalysisModel>> models;
        std::vector<std::unique_ptr<AnalysisConnector>> connectors;
        std::vector<std::unique_ptr<AnalysisConnection>> connections;
        std::vector<std::unique_ptr<AnalysisSystem>> nested_systems;

        AnalysisSystem() = default;

        explicit AnalysisSystem(const std::string &name);

        explicit AnalysisSystem(const ssp4cpp::ssp1::ssd::TSystem &sys, handler::FmuHandler *fmu_handler);

        ~AnalysisSystem();

        AnalysisSystem(AnalysisSystem &&) = default;
        AnalysisSystem &operator=(AnalysisSystem &&) = default;

        AnalysisSystem(const AnalysisSystem &) = delete;
        AnalysisSystem &operator=(const AnalysisSystem &) = delete;

        /// Flat summary of this system.
        std::string to_string() const;

        /// Hierarchical tree view of this system and its contents.
        std::string tree_string() const;

        /// Get all models recursively (flattened).
        std::vector<AnalysisModel *> get_all_models() const;

        /// Get all connections recursively (flattened).
        std::vector<AnalysisConnection *> get_all_connections() const;

        /// Resolve a connector by system path and connector name.
        AnalysisConnector *get_connector(const std::string &system_path,
                                          const std::string &connector_name) const;

        /// Get a nested system by dot-separated path.
        AnalysisSystem *get_nested_system(const std::string &path) const;

        /// Find a connector across all models by model name and full connector name.
        const AnalysisConnector *find_connector(const std::string &model_name,
                                                  const std::string &connector_full_name) const;

        /// Detect algebraic loops using Tarjan SCC (delegates to AnalysisGraphFactory).
        std::vector<std::vector<utils::graph::Node *>> detect_algebraic_loops() const;

        /// Build a transient analysis graph view.
        AnalysisGraphView build_analysis_graph() const;

    private:
        void collect_models(std::vector<AnalysisModel *> &out) const;
        void collect_connections(std::vector<AnalysisConnection *> &out) const;

        std::string tree_string_impl(const std::string &indent) const;
    };
}