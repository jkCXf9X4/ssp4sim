#pragma once


#include "analysis_graph.hpp"
#include "graph.hpp"

#include "utils/data_recorder.hpp"

#include "cutecpp/log.hpp"

namespace ssp4sim::graph
{
    using AnalysisGraph = analysis::graph::AnalysisGraph;

    namespace utils
    {
        class DataRecorder;
    }

    class GraphBuilder
    {
    public:
        Logger log = Logger("ssp4sim.graph.GraphBuilder", LogLevel::info);;

        AnalysisGraph *analysis_graph;
        ssp4sim::utils::DataRecorder *recorder;

        std::map<std::string, std::unique_ptr<Invocable>> models;

        GraphBuilder(AnalysisGraph *ag, ssp4sim::utils::DataRecorder *recorder);

        void build();

        std::unique_ptr<Graph> get_graph();

        std::map<std::string, std::unique_ptr<Invocable>> get_models();

    };

}
