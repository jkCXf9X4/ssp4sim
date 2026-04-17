


#include "SSP1_SystemStructureParameter_Ext.hpp"

#include "ssp4cpp/ssp.hpp"

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
        quill::Logger* log()
        {
            // Cache this logger locally so we avoid eager header initialization.
            static quill::Logger* logger =
                ssp4cpp::utils::log::make_logger("ssp4sim.ext.ssp.ssp1.ssv", quill::LogLevel::TraceL1);
            return logger;
        }
    }

    types::DataType get_parameter_type(ssp4cpp::ssp1::ssv::TParameter &par)
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

    void *get_parameter_value(ssp4cpp::ssp1::ssv::TParameter &par)
    {
        if (par.Boolean.has_value())
        {
            return &par.Boolean.value().value;
        }
        else if (par.Enumeration.has_value())
        {
            return &par.Enumeration.value().value;
        }
        else if (par.Integer.has_value())
        {
            return &par.Integer.value().value;
        }
        else if (par.Real.has_value())
        {
            return &par.Real.value().value;
        }
        else if (par.String.has_value())
        {
            return &par.String.value().value;
        }
        return nullptr;
    }

    std::vector<StartValue> get_start_values(std::vector<ssp4cpp::ParameterBindings> &bindings)
    {
        LOG_TRACE_L1(log(), "[{}] Init", __func__);

        std::vector<StartValue> start_values;
        for (auto &binding : bindings)
        {
            for (auto &parameter : binding.ssv->Parameters.Parameters)
            {
                LOG_TRACE_L1(log(), "[{}] - Store values, {}", __func__, parameter.name);
                StartValue start_value(parameter.name, get_parameter_type(parameter));
                start_value.store_value(get_parameter_value(parameter));

                if (binding.ssm)
                {
                    for (auto &map : binding.ssm->MappingEntry)
                    {
                        if (start_value.name == map.source)
                        {
                            LOG_TRACE_L1(log(), "[{}] Add mapping for {} - {}", __func__, parameter.name, map.target);
                            start_value.mappings.push_back(map.target);
                        }
                    }
                }
                start_values.push_back(std::move(start_value));
            }
        }
        return start_values;
    }

    std::map<std::string, StartValue> get_start_value_map(std::vector<StartValue> &start_values)
    {
        std::map<std::string, StartValue> parameter_map;
        for (auto &value : start_values)
        {
            LOG_TRACE_L1(log(), "[{}] - Parameter {}, {}", __func__, value.name, value.type.to_string());
            for (auto name : value.mappings)
            {
                if (parameter_map.contains(name))
                {
                    LOG_WARNING(log(), "[{}] Overwriting parameter start value for, {}", __func__, name);
                }

                LOG_DEBUG(log(), "[{}] Inserting parameter {} as {}", __func__, value.name, name);
                parameter_map.insert_or_assign(name, value);
                LOG_TRACE_L1(log(), "[{}] - Parameter {} ", __func__, value.to_string());
            }
        }
        return parameter_map;
    }


    std::map<std::string, StartValue> get_start_value_mappings(ssp4cpp::Ssp &ssp)
    {
        // Get initial values
        auto initial_values = get_start_values(ssp.parameter_bindings);
        return get_start_value_map(initial_values);
    }
}
