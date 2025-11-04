#pragma once

#include "cutecpp/log.hpp"

#include "ssp4cpp/utils/string.hpp"

#include "utils/node.hpp"

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

        AnalysisModelVariable();

        AnalysisModelVariable(std::string component, std::string variable_name);

        ~AnalysisModelVariable();

        void update_name();

        std::string get_connector_name() const;

        void print(std::ostream &os) const override;
    };

}
