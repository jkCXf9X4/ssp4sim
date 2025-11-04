#pragma once

#include "utils/node.hpp"
#include "utils/map.hpp"
#include "utils/time.hpp"

#include "FMI2_Enums_Ext.hpp"
#include "SSP1_SystemStructureParameter_Ext.hpp"

#include "cutecpp/log.hpp"

#include "ssp4sim_definitions.hpp"

#include "ssp4cpp/utils/string.hpp"
#include "ssp4cpp/schema/fmi2/FMI2_Enums.hpp"

#include <string>
#include <vector>

namespace ssp4sim::analysis::graph
{

    class AnalysisModel;

    // template class to enable constexpression invoke
    class AnalysisConnector : public ssp4sim::utils::graph::Node
    {
    public:
        uint64_t delay = 0;

        Logger log = Logger("ssp4sim.graph.AnalysisConnector", LogLevel::debug);

        std::string component_name;
        std::string connector_name;

        types::Causality causality;

        uint64_t value_reference;

        types::DataType type;
        std::size_t size;

        // for start value / parameter
        std::unique_ptr<ssp4sim::ext::ssp1::ssv::StartValue> initial_value = nullptr;

        AnalysisModel *model;

        bool forward_derivatives = false;
        int forward_derivatives_order = 0;

        AnalysisConnector();

        AnalysisConnector(std::string component_name,
                          std::string connector_name,
                          uint64_t value_reference,
                          types::DataType type);

        ~AnalysisConnector();

        void update_name();

        static std::string create_name(const std::string &component_name, const std::string &connector_name);

        void print(std::ostream &os) const override;
    };

}
