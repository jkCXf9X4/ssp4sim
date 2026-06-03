#pragma once

#include "ssp4cpp/utils/log.hpp"

#include "utils/node.hpp"

#include "analysis_connector.hpp"

#include "ssp4cpp/fmu.hpp"

#include <cstdint>
#include <string>

namespace ssp4sim::analysis::graph
{
    // intra model connections
    class AnalysisModelVariable : public ssp4sim::utils::graph::Node
    {
        uint64_t delay = 0;

    public:
        ssp4cpp::utils::log::Logger* log = nullptr;
        std::string component;
        std::string variable_name;

        AnalysisModelVariable();

        AnalysisModelVariable(std::string component, std::string variable_name);

        ~AnalysisModelVariable();

        void update_name();

        std::string get_connector_name() const;

        std::string to_string() const override;
    };

    std::map<std::string, std::unique_ptr<AnalysisModelVariable>>
    create_model_variables(std::map<std::string, ssp4cpp::Fmu *> &fmu_map, ssp4cpp::utils::log::Logger *log);

}
