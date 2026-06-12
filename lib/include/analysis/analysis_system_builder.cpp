#include "analysis/analysis_system_builder.hpp"

#include "analysis/components/analysis_model.hpp"
#include "analysis/components/analysis_connector.hpp"
#include "analysis/components/analysis_connection.hpp"
#include "analysis/components/analysis_model_variable.hpp"

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

        ssp4cpp::utils::log::Logger *builder_log()
        {
            static ssp4cpp::utils::log::Logger *logger =
                ssp4cpp::utils::log::make_logger("ssp4sim.analysis.AnalysisSystemBuilder");
            return logger;
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

    /// Recursively walk the SSD tree DFS post-order and apply parameter overrides.
    /// Children first (subsystems, then components), then system-level overrides.
        static int apply_overrides_in_system(
            ssp4cpp::ssp1::ssd::TSystem &ssd_sys,
            ssp4sim::analysis::AnalysisSystem &analysis_sys,
            const ssp4cpp::Ssp *ssp,
            int count = 0)
        {
            // 1. Recurse into subsystems first (DFS post-order)
            if (ssd_sys.Elements.has_value())
            {
                auto &elements = ssd_sys.Elements.value();
                for (size_t i = 0; i < elements.Systems.size() && i < analysis_sys.nested_systems.size(); ++i)
                {
                    count = apply_overrides_in_system(
                        elements.Systems[i],
                        *analysis_sys.nested_systems[i],
                        ssp,
                        count);
                }
            }

            // 2. Component-level overrides
            if (ssd_sys.Elements.has_value())
            {
                auto &elements = ssd_sys.Elements.value();
                for (auto &component : elements.Components)
                {
                    if (!component.ParameterBindings.has_value() || !component.name.has_value())
                        continue;

                    auto comp_map = ext::ssp1::ssv::get_start_value_mappings(
                        component.ParameterBindings->ParameterBindings, ssp);

                    if (comp_map.empty())
                        continue;

                    // Find matching AnalysisModel by component name
                    for (auto &model : analysis_sys.models)
                    {
                        // Match against the last path component (bare name)
                        // since model->name may be path-prefixed (e.g. "inner.sine")
                        std::string model_bare_name = model->name;
                        auto dot_pos = model_bare_name.rfind('.');
                        if (dot_pos != std::string::npos)
                            model_bare_name = model_bare_name.substr(dot_pos + 1);
                        if (model_bare_name != component.name.value())
                            continue;

                        for (auto &connector : model->connectors)
                        {
                            auto key = ssp4sim::analysis::AnalysisConnector::get_connector_name(
                                connector->component_name, connector->connector_name);
                            auto override_val = override_start_value(comp_map, key);

                            // TODO: double check this...
                            /* Fallback: try simple connector name without component prefix
                               (inline component-level bindings without SSM mapping) */
                            if (!override_val)
                            {
                                override_val = override_start_value(comp_map, connector->connector_name);
                            }
                            /* Fallback: try bare component name with connector name
                               (for path-prefixed component names like "inner.sine" -> "sine.f") */
                            if (!override_val)
                            {
                                std::string bare_component = connector->component_name;
                                auto dot_pos = bare_component.rfind('.');
                                if (dot_pos != std::string::npos)
                                {
                                    bare_component = bare_component.substr(dot_pos + 1);
                                    auto bare_key = ssp4sim::analysis::AnalysisConnector::get_connector_name(
                                        bare_component, connector->connector_name);
                                    override_val = override_start_value(comp_map, bare_key);
                                }
                            }
                            if (override_val)
                            {
                                connector->initial_value = std::move(override_val);
                                ++count;
                            }
                        }
                    }
                }
            }

            // 3. System-level overrides (applied after children — naturally overrides them)
            if (ssd_sys.ParameterBindings.has_value())
            {
                auto sys_map = ext::ssp1::ssv::get_start_value_mappings(
                    ssd_sys.ParameterBindings->ParameterBindings, ssp);

                for (auto &model : analysis_sys.models)
                {
                    for (auto &connector : model->connectors)
                    {
                        auto key = ssp4sim::analysis::AnalysisConnector::get_connector_name(
                            connector->component_name, connector->connector_name);
                        auto override_val = override_start_value(sys_map, key);
                        /* Fallback: try bare component name with connector name
                           (for path-prefixed component names like "inner.sine" -> "sine.f") */
                        if (!override_val)
                        {
                            std::string bare_component = connector->component_name;
                            auto dot_pos = bare_component.rfind('.');
                            if (dot_pos != std::string::npos)
                            {
                                bare_component = bare_component.substr(dot_pos + 1);
                                auto bare_key = ssp4sim::analysis::AnalysisConnector::get_connector_name(
                                    bare_component, connector->connector_name);
                                override_val = override_start_value(sys_map, bare_key);
                            }
                        }
                        if (override_val)
                        {
                            connector->initial_value = std::move(override_val);
                            ++count;
                        }
                    }
                }

                // Also override boundary connectors
                for (auto &connector : analysis_sys.connectors)
                {
                    auto key = ssp4sim::analysis::AnalysisConnector::get_connector_name(
                        connector->component_name, connector->connector_name);
                    auto override_val = override_start_value(sys_map, key);
                    // Fallback: try bare connector name
                    if (!override_val)
                    {
                        override_val = override_start_value(sys_map, connector->connector_name);
                    }
                    if (override_val)
                    {
                        connector->initial_value = std::move(override_val);
                        ++count;
                    }
                }
            }

            return count;
        }

    }


    std::unique_ptr<AnalysisSystem> AnalysisSystemBuilder::build(ssp4cpp::Ssp *ssp,
                                                                 handler::FmuHandler *fmu_handler)
    {
        if (!log)
            log = builder_log();
        LOG_TRACE_L1(log, "[{func}] Building AnalysisSystem from SSP", __func__);

        auto analysis_sys = std::make_unique<AnalysisSystem>(ssp->ssd->System, fmu_handler);
        // Apply SSP parameter overrides recursively (DFS post-order: children before parent)
        int override_count = apply_overrides_in_system(
            ssp->ssd->System, *analysis_sys, ssp);
        LOG_TRACE_L1(log, "[{func}] Applied {} SSP parameter overrides", __func__, override_count);

        LOG_TRACE_L1(log, "[{func}] exit", __func__);
        return analysis_sys;
    }

    std::unique_ptr<AnalysisSystem> AnalysisSystemBuilder::build(const std::string &ssp_path)
    {
        if (!log)
            log = builder_log();
        LOG_TRACE_L1(log, "[{func}] Building AnalysisSystem from path {path}", __func__, ssp_path);

        auto ssp = std::make_unique<ssp4cpp::Ssp>(ssp_path);
        auto fmu_handler = std::make_unique<handler::FmuHandler>(ssp.get());
        fmu_handler->init();

        return build(ssp.get(), fmu_handler.get());
    }

} // namespace ssp4sim::analysis
