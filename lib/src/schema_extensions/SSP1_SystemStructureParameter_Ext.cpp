

#include "SSP1_SystemStructureParameter_Ext.hpp"

#include "SSP1_SystemStructureParameterValues_XML.hpp"
#include "SSP1_SystemStructureParameterMapping_XML.hpp"

namespace ssp4cpp::ssp1::ext::ssv
{
    std::vector<StartValue> get_start_values(std::vector<ParameterBindings> &bindings)
    {
        log(trace)("[{}] Init", __func__);

        std::vector<StartValue> start_values;
        for (auto &binding : bindings)
        {
            for (auto &parameter : binding.ssv->Parameters.Parameters)
            {
                log(trace)("[{}] - Store values, {}", __func__, parameter.name);
                StartValue start_value(parameter.name, get_parameter_type(parameter));
                start_value.store_value(get_parameter_value(parameter));

                if (binding.ssm)
                {
                    for (auto &map : binding.ssm->MappingEntry)
                    {
                        if (start_value.name == map.source)
                        {
                            log(trace)("[{}] Add mapping for {} - {}", __func__, parameter.name, map.target);
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
            log(trace)("[{}] - Parameter {}, {}", __func__, value.name, value.type.to_string());
            for (auto name : value.mappings)
            {
                if (parameter_map.contains(name))
                {
                    log(warning)("[{}] Overwriting parameter start value for, {}", __func__, name);
                }

                log(debug)("[{}] Inserting parameter {} as {}", __func__, value.name, name);
                parameter_map.insert_or_assign(name, value);

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
