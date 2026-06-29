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

        std::vector<std::unique_ptr<AnalysisModel>> models;
        std::vector<std::unique_ptr<AnalysisConnector>> connectors;
        std::vector<std::unique_ptr<AnalysisConnection>> connections;
        std::vector<std::unique_ptr<AnalysisSystem>> nested_systems;
        
        std::vector<std::unique_ptr<AnalysisConnection>> resolved_connections;

        std::unique_ptr<AnalysisParameterBindings> bindings;

        AnalysisSystem() = default;

        AnalysisSystem(const ssp4cpp::ssp1::ssd::TSystem &sys, ssp4cpp::Ssp *ssp);

        ~AnalysisSystem();

        AnalysisSystem(AnalysisSystem &&) = default;
        AnalysisSystem &operator=(AnalysisSystem &&) = default;

        AnalysisSystem(const AnalysisSystem &) = delete;
        AnalysisSystem &operator=(const AnalysisSystem &) = delete;

        /// Flat summary of this system.
        std::string to_string() const;
    };
}