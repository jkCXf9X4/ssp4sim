#pragma once
// #include "ssp4sim_definitions.hpp"

#include "ssp4cpp/schema/fmi2/FMI2_Enums.hpp"
#include "ssp4cpp/utils/interface.hpp"

namespace ssp4sim::types
{
    // types::
    using IPrintable = ssp4cpp::utils::interfaces::IPrintable;
    using Causality = ssp4cpp::fmi2::md::Causality;
    using DataType = ssp4cpp::fmi2::md::FmiType;
}




