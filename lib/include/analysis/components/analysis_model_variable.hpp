#pragma once

#include "analysis_component.hpp"

#include "ssp4cpp/schema/fmi2/FMI2_modelDescription.hpp"

#include <string>
#include <vector>

namespace ssp4sim::analysis
{

    class AnalysisModelVariable : public AnalysisComponent
    {
    public:
        std::string component;
        std::string variable_name;
        std::string type;
        unsigned int value_reference;
        ssp4cpp::fmi2::md::Causality causality;
        ssp4cpp::fmi2::md::Variability variability;

        // dependency list
        // This variable depends on the once in the list
        std::vector<ssp4cpp::fmi2::md::fmi2ScalarVariable *> dependencies;

        AnalysisModelVariable() = default;

        AnalysisModelVariable(const ssp4cpp::fmi2::md::fmi2ScalarVariable &var);

        AnalysisModelVariable(std::string component_,
                              std::string variable_name_,
                              std::string type_,
                              std::string /*start_value*/);

        ~AnalysisModelVariable() = default;

        std::string to_string() const;
    };

} // namespace ssp4sim::analysis