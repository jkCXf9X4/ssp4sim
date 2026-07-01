#pragma once


#include "ssp_component.hpp"
#include "ssp_parameter_bindings.hpp"

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
    class SspModel;
    class SspConnector;
    class SspConnection;
    class SspModelVariable;

    class SspSystem : public SspItem
    {
    public:
        std::vector<SspModel> models;
        std::vector<SspConnector> connectors;
        std::vector<SspConnection> connections;
        std::vector<SspSystem> nested_systems;
        
        std::vector<SspConnection> resolved_connections;

        std::map<std::string, ext::ParameterValue> parameter_bindings;

        SspSystem(const ssp4cpp::ssp1::ssd::TSystem &sys, ssp4cpp::Ssp *ssp);

        /// Flat summary of this system.
        std::string to_string() const;
    };
}