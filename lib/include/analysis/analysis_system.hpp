#pragma once

#include "analysis/analysis_graph_view.hpp"

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

        explicit AnalysisSystem(std::string name_);

        ~AnalysisSystem();

        AnalysisSystem(AnalysisSystem &&) = default;
        AnalysisSystem &operator=(AnalysisSystem &&) = default;

        AnalysisSystem(const AnalysisSystem &) = delete;
        AnalysisSystem &operator=(const AnalysisSystem &) = delete;

        /// Build a transient graph view from connectors and model variables.
        AnalysisGraphView build_analysis_graph() const;

        /// Detect algebraic loops using Tarjan's SCC on the graph view.
        std::vector<std::vector<utils::graph::Node *>> detect_algebraic_loops() const;

        /// Recursively collect all models (from this system and nested systems).
        std::vector<AnalysisModel *> get_all_models() const;

        /// Recursively collect all connections (from this system and nested systems),
        /// resolving boundary connectors where applicable.
        std::vector<AnalysisConnection *> get_all_connections() const;

        /// Find a connector by dot-separated path (e.g. "SuT.edrive_mass.M_A").
        AnalysisConnector *get_connector(const std::string &system_path,
                                         const std::string &connector_name) const;

        /// Find a nested system by dot-separated path.
        AnalysisSystem *get_nested_system(const std::string &path) const;

    private:
        void collect_models(std::vector<AnalysisModel *> &out) const;
        void collect_connections(std::vector<AnalysisConnection *> &out) const;
    };
}