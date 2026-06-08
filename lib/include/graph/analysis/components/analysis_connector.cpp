

// DEPRECATED: Use lib/include/analysis/analysis_connector.hpp instead.
// This file is kept for backward compatibility.
#include "graph/analysis/components/analysis_connector.hpp"

#include "handler/fmu_handler.hpp"

#include "SSP1_SystemStructureParameter_Ext.hpp"
#include "FMI2_modelDescription_Ext.hpp"
#include "FMI2_Enums_Ext.hpp"

#include "ssp4cpp/ssp.hpp"

#include <sstream>

namespace ssp4sim::analysis::graph
{

    AnalysisConnector::AnalysisConnector()
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.graph.AnalysisConnector"))
    {
    }

    AnalysisConnector::AnalysisConnector(std::string component_name,
                                         std::string connector_name,
                                         uint64_t value_reference,
                                         types::DataType type)
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.graph.AnalysisConnector"))
    {
        this->component_name = component_name;
        this->connector_name = connector_name;
        update_name();

        this->value_reference = value_reference;
        this->type = type;
        this->size = ssp4sim::ext::fmi2::enums::get_data_type_size(type);
    }

    AnalysisConnector::~AnalysisConnector()
    {
        LOG_TRACE_L1(log, "[{func}] Destroying AnalysisConnector", __func__);
    }

    void AnalysisConnector::update_name()
    {
        this->name = AnalysisConnector::create_name(component_name, connector_name);
    }

    std::string AnalysisConnector::create_name(const std::string &component_name, const std::string &connector_name)
    {
        return component_name + "." + connector_name;
    }

    std::string AnalysisConnector::to_string() const
    {
        std::ostringstream oss;
        oss << "Connector {"
            << "\nname: " << name
            << "\nvr: " << value_reference
            << "\ntype: " << type
            << "\ncausality: " << causality
            << "\n }\n";
        return oss.str();
    }


    std::map<std::string, std::unique_ptr<AnalysisConnector>> create_connectors(ssp4cpp::Ssp &ssp_ref, handler::FmuHandler *fmu_handler, ssp4cpp::utils::log::Logger *log)
    {
        LOG_TRACE_L1(log, "[{func}] init", __func__);
        std::map<std::string, std::unique_ptr<AnalysisConnector>> items;
        if (ssp_ref.ssd->System.Elements.has_value())
        {

            auto mapping_start_values = ssp4sim::ext::ssp1::ssv::get_start_value_mappings(ssp_ref);

            for (auto &component : ssp_ref.ssd->System.Elements.value().Components)
            {
                if (!component.name.has_value())
                {
                    LOG_ERROR(log, "[{func}] Component does not specify name attribute, Its optional but needed for this application {component}", __func__);
                    throw std::runtime_error("Component without name");
                }

                auto component_name = component.name.value();

                if (!fmu_handler->fmu_info_map.contains(component_name))
                {
                    LOG_ERROR(log, "[{func}] Fmu not found, {component}", __func__, component_name);
                    throw std::runtime_error("Fmu not found");
                }
                auto fmu = fmu_handler->fmu_info_map[component_name].get();

                auto md = fmu->model_description;

                auto variables = ext::fmi2::model_variables::get_variables(*md, {types::Causality::input, types::Causality::output, types::Causality::parameter});

                for (auto &var : variables)
                {
                    LOG_DEBUG(log, "[{func}] Creating Connector: {component}.{variable}", __func__, component_name, var.name);
                    auto value_reference = var.valueReference.value();
                    LOG_TRACE_L1(log, "[{func}] get_variable_type {}", __func__, value_reference);
                    auto type = ext::fmi2::model_variables::get_variable_type(var);

                    LOG_TRACE_L1(log, "[{func}] Create AnalysisConnector", __func__);
                    auto c = std::make_unique<AnalysisConnector>(
                        component_name, var.name, value_reference, type);

                    c->causality = var.causality.value(); // it must have value to be selected in the list
                    auto system_name = component_name + "." + var.name;

                    auto start_value = ext::fmi2::model_variables::get_variable_start_value(var);
                    if (start_value)
                    {
                        LOG_DEBUG(log, "[{func}] Applying start value for {name}", __func__, system_name);
                        c->initial_value = std::make_unique<ext::ssp1::ssv::StartValue>(var.name, type);
                        c->initial_value->store_value(start_value);
                    }

                    if (mapping_start_values.contains(system_name))
                    {
                        LOG_DEBUG(log, "[{func}] Applying parameterset value to {name}, {type}", __func__, system_name, type.to_string());

                        const auto &mapped_start_value = mapping_start_values.at(system_name);
                        c->initial_value = std::make_unique<ext::ssp1::ssv::StartValue>(mapped_start_value);
                    }

                    if (c->initial_value)
                    {
                        LOG_DEBUG(log, "[{func}] Initial value {value}", __func__, c->initial_value->to_string());
                    }

                    items[c->name] = std::move(c);
                }
            }
        }
        LOG_DEBUG(log, "[{func}] exit, Total connectors created: {count}", __func__, items.size());
        return items;
    }

}
