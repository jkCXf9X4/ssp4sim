#pragma once

#include "fmu_handler.hpp"

#include "analysis_connection.hpp"
#include "analysis_model.hpp"
#include "analysis_connector.hpp"
#include "analysis_internal.hpp"

#include "analysis_graph.hpp"

#include <map>
#include <memory>
#include <string>

namespace ssp4sim::analysis::graph
{

    class AnalysisGraphBuilder
    {
    public:
        Logger log = Logger("ssp4sim.graph.AnalysisGraphBuilder", LogLevel::info);

        ssp4cpp::Ssp *ssp;
        handler::FmuHandler *fmu_handler;

        AnalysisGraphBuilder(ssp4cpp::Ssp *ssp, handler::FmuHandler *fmu_handler);

        std::map<std::string, std::unique_ptr<AnalysisModel>> create_models(ssp4cpp::Ssp &ssp);

        std::map<std::string, std::unique_ptr<AnalysisConnector>> create_connectors(ssp4cpp::Ssp &ssp);

        std::map<std::string, std::unique_ptr<AnalysisConnection>> create_connections(ssp4cpp::Ssp &ssp);

        std::map<std::string, std::unique_ptr<AnalysisModelVariable>> create_model_variables(std::map<std::string, ssp4cpp::Fmu *> &fmu_map);

        std::unique_ptr<AnalysisGraph> build();
    };

}
