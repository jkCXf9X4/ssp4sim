#include "SSP1_SystemStructureParameter_Ext.hpp"

#include "ssp4sim_definitions.hpp"
#include "ssp4cpp/ssp.hpp"

#include <map>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace ssp4sim::ext::ssp1::ssv
{
    namespace
    {
        ssp4cpp::utils::log::Logger* log()
        {
            // Cache this logger locally so we avoid eager header initialization.
            static ssp4cpp::utils::log::Logger* logger =
                ssp4cpp::utils::log::make_logger("ssp4sim.ext.ssp.ssp1.ssv");
            return logger;
        }
    }

    types::DataType get_parameter_type(const ssp4cpp::ssp1::ssv::TParameter &par)
    {
        if (par.Boolean.has_value())
        {
            return types::DataType::boolean;
        }
        else if (par.Enumeration.has_value())
        {
            return types::DataType::enumeration;
        }
        else if (par.Integer.has_value())
        {
            return types::DataType::integer;
        }
        else if (par.Real.has_value())
        {
            return types::DataType::real;
        }
        else if (par.String.has_value())
        {
            return types::DataType::string;
        }
        else
        {
            throw std::runtime_error("Unknown type");
        }
    }

    void *get_parameter_value(const ssp4cpp::ssp1::ssv::TParameter &par)
    {
        auto &p = const_cast<ssp4cpp::ssp1::ssv::TParameter &>(par);
        if (p.Boolean.has_value())
        {
            return &p.Boolean.value().value;
        }
        else if (p.Enumeration.has_value())
        {
            return &p.Enumeration.value().value;
        }
        else if (p.Integer.has_value())
        {
            return &p.Integer.value().value;
        }
        else if (p.Real.has_value())
        {
            return &p.Real.value().value;
        }
        else if (p.String.has_value())
        {
            return &p.String.value().value;
        }
        return nullptr;
    }

    std::vector<StartValue> get_start_values(
        const std::vector<ssp4cpp::ssp1::ssd::ParameterBinding> &bindings,
        const ssp4cpp::Ssp *ssp)
    {
        LOG_TRACE_L1(log(), "[{func}] Init", __func__);

        std::vector<StartValue> start_values;

        for (auto &binding : bindings)
        {
            // Build SSM mapping lookup for this binding
            std::map<std::string, std::string> ssm_mapping;
            if (binding.ParameterMapping.has_value())
            {
                auto &pm = binding.ParameterMapping.value();

                // External .ssm file reference
                if (pm.source.has_value() && ssp)
                {
                    LOG_TRACE_L1(log(), "[{func}] Loading .ssm file: {source}", __func__, pm.source.value());
                    auto ssm = ssp->load_ssm(pm.source.value());
                    for (auto &entry : ssm.MappingEntry)
                    {
                        ssm_mapping[entry.source] = entry.target;
                    }
                }

                // Inline mapping entries
                if (pm.ParameterMapping.has_value())
                {
                    for (auto &entry : pm.ParameterMapping.value().MappingEntry)
                    {
                        ssm_mapping[entry.source] = entry.target;
                    }
                }
            }

            auto append_parameters = [&start_values, &ssm_mapping](auto &parameters)
            {
                for (auto &parameter : parameters.Parameters)
                {
                    LOG_TRACE_L1(log(), "[{func}] - Store values, {}", __func__, parameter.name);
                    StartValue start_value(parameter.name, get_parameter_type(parameter));
                    start_value.store_value(get_parameter_value(parameter));

                    auto it = ssm_mapping.find(parameter.name);
                    if (it != ssm_mapping.end())
                    {
                        // Remap: parameter is only applied under the mapped name
                        start_value.mappings.clear();
                        start_value.mappings.push_back(it->second);
                    }

                    start_values.push_back(std::move(start_value));
                }
            };

            if (binding.source.has_value() && ssp)
            {
                // Load from external .ssv file
                LOG_TRACE_L1(log(), "[{func}] Loading .ssv file: {source}", __func__, binding.source.value());
                auto param_set = ssp->load_ssv(binding.source.value());
                append_parameters(param_set.Parameters);
            }
            else if (binding.ParameterValues.has_value())
            {
                // Inline parameter values
                append_parameters(binding.ParameterValues.value().ParameterSet.Parameters);
            }
        }
        return start_values;
    }

    std::map<std::string, StartValue> get_start_value_map(
        const std::vector<StartValue> &start_values)
    {
        std::map<std::string, StartValue> parameter_map;
        for (auto &value : start_values)
        {
            LOG_TRACE_L1(log(), "[{func}] - Parameter {}, {}", __func__, value.name, value.type.to_string());
            for (auto name : value.mappings)
            {
                if (parameter_map.contains(name))
                {
                    LOG_WARNING(log(), "[{func}] Overwriting parameter start value for {parameter}", __func__, name);
                }

                LOG_DEBUG(log(), "[{func}] Inserting parameter {value} as {parameter}", __func__, value.name, name);
                parameter_map.insert_or_assign(name, value);
                LOG_TRACE_L1(log(), "[{func}] - Parameter {parameter} ", __func__, value.to_string());
            }
        }
        return parameter_map;
    }

    std::map<std::string, StartValue> get_start_value_mappings(
        const std::vector<ssp4cpp::ssp1::ssd::ParameterBinding> &bindings,
        const ssp4cpp::Ssp *ssp)
    {
        auto start_values = get_start_values(bindings, ssp);
        return get_start_value_map(start_values);
    }


}
