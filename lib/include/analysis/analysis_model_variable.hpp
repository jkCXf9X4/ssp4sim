#pragma once

#include <string>

namespace ssp4sim::analysis
{

    class AnalysisModelVariable
    {
    public:
        std::string name;
        std::string component;
        std::string variable_name;
        std::string type;
        std::string value_reference;
        std::string causality;
        std::string variability;

        AnalysisModelVariable() = default;

        AnalysisModelVariable(std::string component_,
                              std::string variable_name_,
                              std::string type_ = "",
                              std::string value_reference_ = "");

        ~AnalysisModelVariable() = default;

        std::string to_string() const;
    };

} // namespace ssp4sim::analysis