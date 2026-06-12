#pragma once

#include "shared_config.hpp"
#include "graph.hpp"

#include "analysis/analysis_graph_factory.hpp"
#include "analysis/components/analysis_system.hpp"

#include <map>
#include <memory>
#include <string>

namespace ssp4sim::graph
{

    class GraphBuilder
    {
    public:
        ssp4cpp::utils::log::Logger *log = nullptr;

        const analysis::AnalysisSystem &analysis_system;
        ssp4sim::signal::DataRecorder *recorder;
        ssp4sim::SharedConfig *config;

        std::map<std::string, std::unique_ptr<Invocable>> models;

        GraphBuilder(const analysis::AnalysisSystem &analysis_system_,
                     ssp4sim::signal::DataRecorder *recorder,
                     ssp4sim::SharedConfig *config);

        /// Build the simulation graph using pre-resolved graph data (no string matching).
        void build_with_data(analysis::AnalysisGraphData &graph_data);

        std::unique_ptr<Graph> get_graph();

        std::map<std::string, std::unique_ptr<Invocable>> get_models();

    private:
        void create_fmu_models();
        void create_data_storage_areas();
        void wire_connections(analysis::AnalysisGraphData &graph_data);
        void derive_model_edges(analysis::AnalysisGraphData &graph_data);
    };

}