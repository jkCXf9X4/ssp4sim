#pragma once

#include "parameter_value.hpp"
#include "ssp4sim_definitions.hpp"
#include "ssp4cpp/schema/ssp1/SSP1_SystemStructureDescription.hpp"
#include "ssp4cpp/utils/log.hpp"

#include <map>
#include <string>
#include <vector>

namespace ssp4cpp
{
    class Ssp;
}

namespace ssp4sim::ext::ssp1::ssv
{

    types::DataType get_parameter_type(const ssp4cpp::ssp1::ssv::TParameter &par);
    void *get_parameter_value(const ssp4cpp::ssp1::ssv::TParameter &par);

    std::map<std::string, ssp4cpp::ssp1::ssv::TParameter> get_parameter_mapping(
        const std::vector<ssp4cpp::ssp1::ssd::ParameterBinding> &bindings,
        const ssp4cpp::Ssp *ssp);

    std::map<std::string, std::unique_ptr<ext::ParameterValue>> get_start_value_mappings(
        const std::vector<ssp4cpp::ssp1::ssd::ParameterBinding> &bindings,
        const ssp4cpp::Ssp *ssp);

}
