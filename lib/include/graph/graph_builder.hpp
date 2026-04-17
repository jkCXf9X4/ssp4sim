#pragma once


#include "analysis_graph.hpp"
#include "graph.hpp"

#include "signal/recorder.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <map>
#include <memory>
#include <string>

namespace ssp4sim::graph
{
    using AnalysisGraph = analysis::graph::AnalysisGraph;


    class GraphBuilder
    {
    public:
        quill::Logger* log = ssp4cpp::utils::log::make_logger("ssp4sim.graph.GraphBuilder", quill::LogLevel::TraceL1);;

        AnalysisGraph *analysis_graph;
        ssp4sim::signal::DataRecorder *recorder;

        std::map<std::string, std::unique_ptr<Invocable>> models;

        GraphBuilder(AnalysisGraph *ag, ssp4sim::signal::DataRecorder *recorder);

        void build();

        std::unique_ptr<Graph> get_graph();

        std::map<std::string, std::unique_ptr<Invocable>> get_models();

    };

}
