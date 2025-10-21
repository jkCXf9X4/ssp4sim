#pragma once

#include "utils/node.hpp"
#include "utils/map.hpp"
#include "utils/string.hpp"
#include "utils/time.hpp"
#include "utils/log.hpp"

#include "analysis_connector.hpp"

#include <string>
#include <vector>

namespace ssp4sim::analysis::graph
{
    // intra model connections
    class AnalysisModelVariable : public ssp4sim::utils::graph::Node
    {
        uint64_t delay = 0;

    public:
        Logger log = Logger("ssp4sim.graph.AnalysisModelVariable", LogLevel::debug);
        std::string component;
        std::string variable_name;

        AnalysisModelVariable() {}

        AnalysisModelVariable(std::string component, std::string variable_name)
        {
            this->component = component;
            this->variable_name = variable_name;
            update_name();
        }

        ~AnalysisModelVariable()
        {
            log(ext_trace)("[{}] Destroying AnalysisModelVariable", __func__);
        }

        void update_name()
        {
            this->name = AnalysisConnector::create_name(component, variable_name);
        }

        std::string get_connector_name()
        {
            return AnalysisConnector::create_name(component, variable_name);
        }

        virtual void print(std::ostream &os) const
        {
            os << "AnalysisModelVariable {"
               << "\nname: " << name
               << "\ncomponent: " << component
               << "\nvariable_name: " << variable_name
               << "\n}\n";
        }
    };

}