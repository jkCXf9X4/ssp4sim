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
        ssp4cpp::utils::log::Logger *log()
        {
            // Cache this logger locally so we avoid eager header initialization.
            static ssp4cpp::utils::log::Logger *logger =
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

    // https://ssp-standard.org/docs/2.0.1/#_parameterbindings
    // When no parameter mapping is specified as part of the binding, then all the parameter values provided by the parameter source are applied using their original names. If a parameter matching this name is found in the system, the parameter value is applied. Otherwise that parameter value is ignored.

    // When a parameter mapping is specified as part of the binding, then only the mapped parameter values are applied, using their mapped-to names. Non-mapped parameter values are not applied in this case.

    std::map<std::string, ssp4cpp::ssp1::ssv::TParameter> get_parameter_mapping(
        const std::vector<ssp4cpp::ssp1::ssd::ParameterBinding> &bindings,
        const ssp4cpp::Ssp *ssp)
    {
        LOG_TRACE_L1(log(), "[{func}] Init", __func__);

        std::map<std::string, ssp4cpp::ssp1::ssv::TParameter> mapping;
        for (auto &binding : bindings)
        {
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

            // Build SSM mapping lookup for this binding
            std::vector<std::pair<std::string, std::string>> ssm_mapping;
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
                        ssm_mapping.push_back(std::make_pair(entry.source, entry.target));
                    }
                }

                // Inline mapping entries
                if (pm.ParameterMapping.has_value())
                {
                    for (auto &entry : pm.ParameterMapping.value().MappingEntry)
                    {
                        ssm_mapping.push_back(std::make_pair(entry.source, entry.target));
                    }
                }

                // Only map those with mapping if mapping exists
                for (auto parameter : params->Parameters)
                {
                    for (auto &[source, target] : ssm_mapping)
                    {
                        // only add the once that have matching names according to the standard
                        if (source == parameter.name)
                        {
                            mapping[target] = parameter;
                        }
                    }
                }
            }
            else
            {
                // no parameter mapping - add all according to name
                for (auto parameter : params->Parameters)
                {
                    mapping[parameter.name] = parameter;
                }
            }
        }
        return mapping;
    }

    std::map<std::string, ext::ParameterValue> get_start_value_mappings(
        const std::vector<ssp4cpp::ssp1::ssd::ParameterBinding> &bindings,
        const ssp4cpp::Ssp *ssp)
    {
        std::map<std::string, ext::ParameterValue> result;

        auto map = get_parameter_mapping(bindings, ssp);

        for (auto &[name, parameter] : map)
        {
            auto start_value = ParameterValue(parameter.name, get_parameter_type(parameter));
            start_value.store_value(get_parameter_value(parameter));

            result[name] = std::move(start_value);
        }

        return result;
    }

}
