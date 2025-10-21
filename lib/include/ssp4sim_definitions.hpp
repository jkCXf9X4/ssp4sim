#pragma once

#include "FMI2_Enums.hpp"

namespace ssp4sim::sim
{
    using Causality = ssp4sim::fmi2::md::Causality;
    using DataType = ssp4sim::fmi2::md::Type;
}


// log_toggle.hpp
#ifdef _LOG_
  #define IF_LOG(stmt) do { stmt } while (0)
#else
  #define IF_LOG(stmt) do { } while (0)
#endif