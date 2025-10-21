#pragma once

#include "utils/node.hpp"
#include "utils/map.hpp"
#include "utils/string.hpp"
#include "utils/time.hpp"
#include "utils/log.hpp"

#include "FMI2_Enums.hpp"
#include "FMI2_Enums_Ext.hpp"
#include "SSP1_SystemStructureParameter_Ext.hpp"

#include "sim.hpp"

#include <string>
#include <vector>

namespace ssp4sim::analysis::graph
{

    class AnalysisModel;

    // template class to enable constexpression invoke
    class AnalysisConnector : public ssp4sim::utils::graph::Node
    {
        uint64_t delay = 0;

    public:
        Logger log = Logger("ssp4sim.graph.AnalysisConnector", LogLevel::debug);

        std::string component_name;
        std::string connector_name;

        Causality causality;

        uint64_t value_reference;

        DataType type;
        std::size_t size;

        // for start value / parameter
        std::unique_ptr<ssp4cpp::ssp1::ext::ssv::StartValue> initial_value;

        AnalysisModel *model;

        bool forward_derivatives = false;
        int forward_derivatives_order = 0;

        AnalysisConnector()
        {
        }

        AnalysisConnector(std::string component_name,
                          std::string connector_name,
                          uint64_t value_reference,
                          DataType type)
        {
            this->component_name = component_name;
            this->connector_name = connector_name;
            update_name();

            this->value_reference = value_reference;
            this->type = type;
            this->size = ssp4cpp::fmi2::ext::enums::get_data_type_size(type);
        }

        virtual ~AnalysisConnector()
        {
            log(ext_trace)("[{}] Destroying AnalysisConnector", __func__);
        }

        void update_name()
        {
            this->name = AnalysisConnector::create_name(component_name, connector_name);
        }

        static std::string create_name(std::string component_name, std::string connector_name)
        {
            return component_name + "." + connector_name;
        }

        virtual void print(std::ostream &os) const
        {
            os << "Connector {"
               << "\nname: " << name
               << "\nvr: " << value_reference
               << "\ntype: " << type
               << "\ncausality: " << causality
               << "\n }\n";
        }
    };

}