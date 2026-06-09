#pragma once

#include "analysis/analysis_graph_view.hpp"

#include "handler/fmu_handler.hpp"

#include "SSP1_SystemStructureDescription.hpp"

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

    private:
        void collect_models(std::vector<AnalysisModel *> &out) const;
        void collect_connections(std::vector<AnalysisConnection *> &out) const;

        std::string tree_string_impl(const std::string &indent) const;
    };
}