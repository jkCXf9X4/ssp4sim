#pragma once

#include "shared_config.hpp"
#include "analysis_graph.hpp"
#include "graph.hpp"

#include <map>
#include <memory>
#include <string>

namespace ssp4sim::graph
{
    using AnalysisGraph = analysis::graph::AnalysisGraph;

    class GraphBuilder
    {
    public:
        ssp4cpp::utils::log::Logger *log = nullptr;

        AnalysisGraph *analysis_graph;
        ssp4sim::signal::DataRecorder *recorder;
        ssp4sim::SharedConfig *config;

        std::map<std::string, std::unique_ptr<Invocable>> models;

        GraphBuilder(AnalysisGraph *ag, ssp4sim::signal::DataRecorder *recorder, ssp4sim::SharedConfig *config);

        void build();

        std::unique_ptr<Graph> get_graph();

        std::map<std::string, std::unique_ptr<Invocable>> get_models();

    private:
        void create_fmu_models();
        void create_data_storage_areas();
        void wire_connections();
    };

}
