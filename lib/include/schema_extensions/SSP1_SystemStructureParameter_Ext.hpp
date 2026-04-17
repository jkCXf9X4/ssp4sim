

#pragma once

#include "initial_value.hpp"

#include "ssp4sim_definitions.hpp"

#include "ssp4cpp/schema/ssp1/SSP1_SystemStructureParameterValues.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <map>
#include <string>

namespace ssp4cpp
{
    class Ssp;
}

namespace ssp4sim::ext::ssp1::ssv
{
    types::DataType get_parameter_type(ssp4cpp::ssp1::ssv::TParameter &par);

    void *get_parameter_value(ssp4cpp::ssp1::ssv::TParameter &par);

    std::map<std::string, StartValue> get_start_value_mappings(ssp4cpp::Ssp &ssp);

}
