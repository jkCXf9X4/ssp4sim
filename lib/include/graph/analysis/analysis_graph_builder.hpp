#pragma once

#include "utils/map.hpp"
#include "utils/vector.hpp"
#include "utils/time.hpp"

#include "SSP_Ext.hpp"
#include "SSP1_SystemStructureDescription_Ext.hpp"

#include "FMI2_modelDescription_Ext.hpp"
#include "FMI2_Enums_Ext.hpp"

#include "fmu_handler.hpp"

#include "data_type.hpp"

#include "analysis_connection.hpp"
#include "analysis_model.hpp"
#include "analysis_connector.hpp"
#include "analysis_internal.hpp"

#include "analysis_graph.hpp"

#include "ssp4sim_definitions.hpp"

#include <list>
#include <map>
#include <set>
#include <vector>

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

