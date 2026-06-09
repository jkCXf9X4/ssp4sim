#pragma once

#include "initial_value.hpp"
#include "ssp4sim_definitions.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace ssp4sim::analysis
{

    class AnalysisSystem;

    class AnalysisSysConnector
    {
    public:

        std::string name;

        std::string system_name;
        std::string connector_name;

        types::Causality causality;

        bool is_feedthrough = false;

        AnalysisSysConnector() = default;

        AnalysisSysConnector(std::string system_name_,
                          std::string connector_name_,
                          types::Causality causality_);

        ~AnalysisSysConnector();

        AnalysisSysConnector(AnalysisSysConnector &&) = default;
        AnalysisSysConnector &operator=(AnalysisSysConnector &&) = default;

        AnalysisSysConnector(const AnalysisSysConnector &) = delete;
        AnalysisSysConnector &operator=(const AnalysisSysConnector &) = delete;

        static std::string get_connector_name(const std::string &component_name,
                                        const std::string &connector_name_);

        std::string to_string() const;
    };

} // namespace ssp4sim::analysis