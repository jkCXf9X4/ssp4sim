#pragma once

#include "_ssp_item.hpp"

#include "ssp4cpp/schema/fmi2/FMI2_modelDescription.hpp"

#include <string>
#include <vector>

namespace ssp4sim::analysis
{

    class SspModelVariable : public SspItem
    {
    public:
        unsigned int value_reference;
        ssp4cpp::fmi2::md::Causality causality;
        ssp4cpp::fmi2::md::Variability variability;

        // dependency list
        // This variable depends on the once in the list
        std::vector<ssp4cpp::fmi2::md::fmi2ScalarVariable *> dependencies;

        SspModelVariable(const ssp4cpp::fmi2::md::fmi2ScalarVariable &var);

        std::string to_string() const;
    };

} // namespace ssp4sim::analysis