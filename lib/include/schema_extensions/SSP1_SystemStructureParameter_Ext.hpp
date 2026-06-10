#pragma once

#include "initial_value.hpp"
#include "ssp4sim_definitions.hpp"
#include "ssp4cpp/schema/ssp1/SSP1_SystemStructureDescription.hpp"
#include "ssp4cpp/utils/log.hpp"

#include <map>
#include <string>
#include <vector>

namespace ssp4sim::ext::ssp1::ssv
{
    types::DataType get_parameter_type(const ssp4cpp::ssp1::ssv::TParameter &par);
    void *get_parameter_value(const ssp4cpp::ssp1::ssv::TParameter &par);

    std::map<std::string, StartValue> get_start_value_mappings(
        const std::vector<ssp4cpp::ssp1::ssd::ParameterBinding> &bindings);

    std::map<std::string, StartValue> apply_parameter_mappings(
        ssp4cpp::ssp1::ssd::TSystem &system);
}
