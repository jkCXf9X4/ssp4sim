#include "SSP1_SystemStructureParameter_Ext.hpp"

#include "ssp4sim_definitions.hpp"

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
        const std::vector<ssp4cpp::ssp1::ssd::ParameterBinding> &bindings)
    {
        LOG_TRACE_L1(log(), "[{func}] Init", __func__);

        std::vector<StartValue> start_values;
        for (auto &binding : bindings)
        {
            if (!binding.ParameterValues.has_value())
                continue;
            for (auto &parameter : binding.ParameterValues.value().ParameterSet.Parameters.Parameters)
            {
                LOG_TRACE_L1(log(), "[{func}] - Store values, {}", __func__, parameter.name);
                StartValue start_value(parameter.name, get_parameter_type(parameter));
                start_value.store_value(get_parameter_value(parameter));

                start_values.push_back(std::move(start_value));
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
        const std::vector<ssp4cpp::ssp1::ssd::ParameterBinding> &bindings)
    {
        auto start_values = get_start_values(bindings);
        return get_start_value_map(start_values);
    }

    std::map<std::string, StartValue> apply_parameter_mappings(
        ssp4cpp::ssp1::ssd::TSystem &system)
    {
        std::map<std::string, StartValue> result;

        if (system.Elements.has_value())
        {
            // Recurse into nested systems first (children)
            for (auto &subsystem : system.Elements->Systems)
            {
                auto sub_map = apply_parameter_mappings(subsystem);
                result.merge(sub_map);  // merge inserts only keys not already in result
            }

            // Apply component-level bindings
            for (auto &component : system.Elements->Components)
            {
                if (component.ParameterBindings.has_value())
                {
                    auto comp_map = get_start_value_mappings(
                        component.ParameterBindings->ParameterBindings);
                    result.merge(comp_map);
                }
            }
        }

        // System-level bindings: overrides all children (both subsystems and components)
        if (system.ParameterBindings.has_value())
        {
            auto sys_map = get_start_value_mappings(
                system.ParameterBindings->ParameterBindings);
            for (auto &[key, value] : sys_map)
            {
                result.insert_or_assign(key, std::move(value));
            }
        }

        return result;
    }
}
