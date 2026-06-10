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
            std::map<std::string, std::vector<std::string>> ssm_mapping;
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
                        ssm_mapping[entry.source].push_back(entry.target);
                    }
                }

                // Inline mapping entries
                if (pm.ParameterMapping.has_value())
                {
                    for (auto &entry : pm.ParameterMapping.value().MappingEntry)
                    {
                        ssm_mapping[entry.source].push_back(entry.target);
                    }
                }
            }

            // Resolve parameter source - external file or inline
            const ssp4cpp::ssp1::ssv::TParameters *params = nullptr;
            ssp4cpp::ssp1::ssv::ParameterSet param_set_holder;
            if (binding.source.has_value() && ssp)
            {
                // Load from external .ssv file
                LOG_TRACE_L1(log(), "[{func}] Loading .ssv file: {source}", __func__, binding.source.value());
                param_set_holder = ssp->load_ssv(binding.source.value());
                params = &param_set_holder.Parameters;
            }
            else if (binding.ParameterValues.has_value())
            {
                // Inline parameter values
                params = &binding.ParameterValues.value().ParameterSet.Parameters;
            }

            if (!params)
                continue;

            // Single parameter handling path
            for (auto &parameter : params->Parameters)
            {
                LOG_TRACE_L1(log(), "[{func}] - Store values, {}", __func__, parameter.name);
                StartValue start_value(parameter.name, get_parameter_type(parameter));
                start_value.store_value(get_parameter_value(parameter));

                auto it = ssm_mapping.find(parameter.name);
                if (it != ssm_mapping.end())
                {
                    start_value.mappings = it->second;
                }

                start_values.push_back(std::move(start_value));
            }
        }
        return start_values;
    }

    std::map<std::string, StartValue> get_start_value_mappings(
        const std::vector<ssp4cpp::ssp1::ssd::ParameterBinding> &bindings,
        const ssp4cpp::Ssp *ssp)
    {
        auto start_values = get_start_values(bindings, ssp);

        std::map<std::string, StartValue> result;
        for (auto &value : start_values)
        {
            if (value.mappings.empty())
            {
                /* Without SSM mappings, use the parameter name itself as the key.
                   For system-level bindings this is "component.connector",
                   for component-level bindings this is the simple connector name. */
                result.insert_or_assign(value.name, std::move(value));
            }
            else
            {
                for (auto name : value.mappings)
                {
                    LOG_TRACE_L1(log(), "[{func}] Name: {name}", __func__, name);
                    if (result.find(name) != result.end())
                    {
                        LOG_WARNING(log(), "Overwriting parameter: {name}", name);
                    }
                    result.insert_or_assign(name, std::move(value));
                }
            }
        }
        return result;
    }


}
