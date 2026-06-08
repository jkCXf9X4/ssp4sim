#include "analysis/analysis_system_builder.hpp"

#include "analysis/analysis_model.hpp"
#include "analysis/analysis_connector.hpp"
#include "analysis/analysis_connection.hpp"
#include "analysis/analysis_model_variable.hpp"

#include "handler/fmu_handler.hpp"

#include "SSP1_SystemStructureDescription_Ext.hpp"
#include "SSP1_SystemStructureParameter_Ext.hpp"
#include "SSP_Ext.hpp"
#include "FMI2_modelDescription_Ext.hpp"
#include "FMI2_Enums_Ext.hpp"
#include "utils/time.hpp"

#include "ssp4cpp/ssp.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace ssp4sim::analysis
{

    namespace
    {
        ssp4cpp::utils::log::Logger* builder_log()
        {
            static ssp4cpp::utils::log::Logger* logger =
                ssp4cpp::utils::log::make_logger("ssp4sim.analysis.AnalysisSystemBuilder");
            return logger;
        }

/// Build an initial_value from an FMI variable start value, or return nullptr.
        static std::unique_ptr<ext::ssp1::ssv::StartValue>
        make_start_value(const std::string &var_name,
                         types::DataType type,
                         void *start_val)
        {
            if (!start_val)
                return nullptr;
            auto sv = std::make_unique<ext::ssp1::ssv::StartValue>(var_name, type);
            sv->store_value(start_val);
            return sv;
        }

        /// Apply an SSP parameter-set override, returning a new StartValue or nullptr.
        static std::unique_ptr<ext::ssp1::ssv::StartValue>
        override_start_value(const std::map<std::string, ext::ssp1::ssv::StartValue> &mappings,
                             const std::string &system_name)
        {
            auto it = mappings.find(system_name);
            if (it == mappings.end())
                return nullptr;
            return std::make_unique<ext::ssp1::ssv::StartValue>(it->second);
        }

/// Create FMU-level connectors for a given component and FMU info.
        std::vector<std::unique_ptr<AnalysisConnector>>
        create_fmu_connectors(const std::string &component_name,
                                handler::FmuInfo *fmu_info,
                                const std::map<std::string, ext::ssp1::ssv::StartValue> &parameter_mappings)
        {
            std::vector<std::unique_ptr<AnalysisConnector>> connectors;
            auto md = fmu_info->model_description;

            auto variables = ext::fmi2::model_variables::get_variables(
                *md, {types::Causality::input, types::Causality::output, types::Causality::parameter});

            for (auto &var : variables)
            {
                auto value_reference = var.valueReference.value();
                auto type = ext::fmi2::model_variables::get_variable_type(var);

                auto c = std::make_unique<AnalysisConnector>(
                    component_name, var.name, value_reference, type);

                c->causality = var.causality.value();
                c->is_boundary = false;

                auto system_name = component_name + "." + var.name;

                auto start_value = ext::fmi2::model_variables::get_variable_start_value(var);
                auto iv = make_start_value(var.name, type, start_value);
                if (iv)
                    c->initial_value = std::move(iv);

                // SSP parameter set overrides supersede FMU-provided start values
                auto override_iv = override_start_value(parameter_mappings, system_name);
                if (override_iv)
                    c->initial_value = std::move(override_iv);

                connectors.push_back(std::move(c));
            }

            return connectors;
        }

        /// Build an AnalysisSystem hierarchy recursively from a TSystem node.
        std::unique_ptr<AnalysisSystem>
        build_system_recursive(const ssp4cpp::ssp1::ssd::TSystem &sys,
                                handler::FmuHandler *fmu_handler,
                                const std::map<std::string, ext::ssp1::ssv::StartValue> &parameter_mappings,
                                AnalysisSystemBuilder &builder)
        {
            auto analysis_sys = std::make_unique<AnalysisSystem>(
                sys.name.value_or("unnamed"));

            if (!sys.Elements.has_value())
            {
                return analysis_sys;
            }

            auto &elements = sys.Elements.value();

            // Process system-level (boundary) connectors
            builder.process_boundary_connectors(*analysis_sys, sys);

            // Process Components (FMU models)
            for (auto &component : elements.Components)
            {
                if (!component.name.has_value())
                {
                    LOG_ERROR(builder_log(), "[{func}] Component does not specify name attribute", __func__);
                    throw std::runtime_error("Component without name");
                }

                auto component_name = component.name.value();

                if (!fmu_handler->fmu_info_map.contains(component_name))
                {
                    LOG_ERROR(builder_log(), "[{func}] FMU not found: {name}", __func__, component_name);
                    throw std::runtime_error("FMU not found: " + component_name);
                }

                auto fmu_info = fmu_handler->fmu_info_map[component_name].get();

                auto model = std::make_unique<AnalysisModel>(
                    component_name, component.source, fmu_info);

                // Set interpolation data from FMU CoSimulation attributes
                if (fmu_info->model_description->CoSimulation)
                {
                    auto &co_sim = *fmu_info->model_description->CoSimulation;
                    model->canInterpolateInputs = co_sim.canInterpolateInputs.value_or(false);
                    model->maxOutputDerivativeOrder = co_sim.maxOutputDerivativeOrder.value_or(0);
                }

                // Create FMU-level connectors from the model description
                model->connectors = create_fmu_connectors(component_name, fmu_info, parameter_mappings);

                // Compute feedthrough from FMU ModelStructure
                model->compute_feedthrough(fmu_info);

                analysis_sys->models.push_back(std::move(model));
            }

            // Process nested Systems (recursive)
            for (auto &sub_sys : elements.Systems)
            {
                auto nested = build_system_recursive(sub_sys, fmu_handler, parameter_mappings, builder);
                analysis_sys->nested_systems.push_back(std::move(nested));
            }

            // Process connections at this system level
            builder.process_connections(*analysis_sys, sys);

            return analysis_sys;
        }
    }

    // -----------------------------------------------------------------------
    // Private member implementations
    // -----------------------------------------------------------------------

    void AnalysisSystemBuilder::process_boundary_connectors(
        AnalysisSystem &analysis_sys,
        const ssp4cpp::ssp1::ssd::TSystem &sys)
    {
        if (!sys.Connectors.has_value())
            return;

        for (const auto &conn : sys.Connectors.value().Connectors)
        {
            auto boundary_conn = std::make_unique<AnalysisConnector>();
            boundary_conn->name = analysis_sys.name + "." + conn.name;
            boundary_conn->type_str = "IO";
            boundary_conn->causality = conn.kind;
            boundary_conn->is_boundary = true;
            analysis_sys.connectors.push_back(std::move(boundary_conn));
        }
    }

    void AnalysisSystemBuilder::process_connections(
        AnalysisSystem &analysis_sys,
        const ssp4cpp::ssp1::ssd::TSystem &sys)
    {
        if (!sys.Connections.has_value())
            return;

        for (const auto &connection : sys.Connections.value().Connections)
        {
            auto delay = utils::time::s_to_ns(connection.information_delay.value_or(0));
            bool is_boundary = !connection.startElement.has_value() ||
                               !connection.endElement.has_value();
            std::string src_model = connection.startElement.value_or(analysis_sys.name);
            std::string tgt_model = connection.endElement.value_or(analysis_sys.name);

            auto conn = std::make_unique<AnalysisConnection>(
                src_model, connection.startConnector,
                tgt_model, connection.endConnector,
                delay, is_boundary);
            analysis_sys.connections.push_back(std::move(conn));
        }
    }

    std::unique_ptr<AnalysisSystem> AnalysisSystemBuilder::build(const std::string &ssp_path)
    {
        if (!log) log = builder_log();
        LOG_TRACE_L1(log, "[{func}] Building AnalysisSystem from {path}", __func__, ssp_path);

        auto ssp = std::make_unique<ssp4cpp::Ssp>(ssp_path);
        auto fmu_handler = std::make_unique<handler::FmuHandler>(ssp.get());
        fmu_handler->init();

        auto param_mappings = ext::ssp1::ssv::get_start_value_mappings(*ssp);
        auto result = build_system_recursive(ssp->ssd->System, fmu_handler.get(), param_mappings, *this);

        LOG_TRACE_L1(log, "[{func}] exit", __func__);
        return result;
    }

std::unique_ptr<AnalysisSystem> AnalysisSystemBuilder::build(ssp4cpp::Ssp *ssp,
                                                                    handler::FmuHandler *fmu_handler)
    {
        if (!log) log = builder_log();
        LOG_TRACE_L1(log, "[{func}] Building AnalysisSystem from SSP", __func__);

        auto param_mappings = ext::ssp1::ssv::get_start_value_mappings(*ssp);
        auto result = build_system_recursive(ssp->ssd->System, fmu_handler, param_mappings, *this);

        LOG_TRACE_L1(log, "[{func}] exit", __func__);
        return result;
    }

} // namespace ssp4sim::analysis