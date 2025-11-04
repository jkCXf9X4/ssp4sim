#pragma once

#include "cutecpp/log.hpp"

#include "FMI2_Enums_Ext.hpp"

#include "analysis_graph.hpp"
#include "data_recorder.hpp"

#include "model_fmu.hpp"
#include "graph.hpp"

#include "ssp4sim_definitions.hpp"

#include "utils/map.hpp"

#include <map>

namespace ssp4sim::graph
{
    using AnalysisGraph = analysis::graph::AnalysisGraph;

    class GraphBuilder
    {
    public:
        Logger log = Logger("ssp4sim.graph.GraphBuilder", LogLevel::info);;

        AnalysisGraph *analysis_graph;
        utils::DataRecorder *recorder;

        std::map<std::string, std::unique_ptr<Invocable>> models;

        GraphBuilder(AnalysisGraph *ag, utils::DataRecorder *recorder);

        void build();

        std::unique_ptr<Graph> get_graph();

        std::map<std::string, std::unique_ptr<Invocable>> get_models();

    };

}
