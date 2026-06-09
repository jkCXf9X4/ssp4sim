#pragma once

#include "ssp4cpp/schema/fmi2/FMI2_modelDescription.hpp"

#include <string>

namespace ssp4sim::analysis
{

    class AnalysisModelVariable
    {
    public:
        std::string name;
        unsigned int value_reference;
        ssp4cpp::fmi2::md::Causality causality;
        ssp4cpp::fmi2::md::Variability variability;

        // dependency list
        // This variable depends on the once in the list
        std::vector<ssp4cpp::fmi2::md::fmi2ScalarVariable *> dependencies;

        AnalysisModelVariable() = default;

        AnalysisModelVariable(const ssp4cpp::fmi2::md::fmi2ScalarVariable &var);

        ~AnalysisModelVariable() = default;

        std::string to_string() const;
    };

} // namespace ssp4sim::analysis