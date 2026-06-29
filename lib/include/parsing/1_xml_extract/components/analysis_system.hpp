#pragma once


#include "analysis_component.hpp"
#include "analysis_parameter_bindings.hpp"

#include "ssp4cpp/schema/ssp1/SSP1_SystemStructureDescription.hpp"
#include "ssp4cpp/ssp.hpp"

#include "utils/node.hpp"
#include "utils/tarjan.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <optional>

namespace ssp4sim::analysis
{
    // Forward declarations for data objects
    class AnalysisModel;
    class AnalysisConnector;
    class AnalysisConnection;
    class AnalysisModelVariable;

    class AnalysisSystem : public AnalysisComponent
    {
    public:
        ssp4cpp::Ssp *ssp;

        std::vector<AnalysisModel> models;
        std::vector<AnalysisConnector> connectors;
        std::vector<AnalysisConnection> connections;
        std::vector<AnalysisSystem> nested_systems;
        
        std::vector<AnalysisConnection> resolved_connections;

        std::map<std::string, ext::ParameterValue> parameter_bindings;

        AnalysisSystem(const ssp4cpp::ssp1::ssd::TSystem &sys, ssp4cpp::Ssp *ssp);

        /// Flat summary of this system.
        std::string to_string() const;
    };
}