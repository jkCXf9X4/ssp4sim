#pragma once

#include "parameter_value.hpp"

#include "SSP1_SystemStructureDescription.hpp"
#include "ssp.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>
#include <map>


namespace ssp4sim::analysis
{

    class AnalysisParameterBindings
    {
    public:

        std::map<std::string, std::unique_ptr<ext::ParameterValue>> parameter_bindings;

        AnalysisParameterBindings() = default;

        AnalysisParameterBindings(std::optional<ssp4cpp::ssp1::ssd::TParameterBindings> bindings, ssp4cpp::Ssp *ssp);

        ~AnalysisParameterBindings();

        AnalysisParameterBindings(AnalysisParameterBindings &&) = default;
        AnalysisParameterBindings &operator=(AnalysisParameterBindings &&) = default;

        AnalysisParameterBindings(const AnalysisParameterBindings &) = delete;
        AnalysisParameterBindings &operator=(const AnalysisParameterBindings &) = delete;

        std::string to_string() const;

    };

} // namespace ssp4sim::analysis