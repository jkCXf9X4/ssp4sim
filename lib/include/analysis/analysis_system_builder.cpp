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

    }


    std::unique_ptr<AnalysisSystem> AnalysisSystemBuilder::build(ssp4cpp::Ssp *ssp,
                                                                 handler::FmuHandler *fmu_handler)
    {
        if (!log)
            log = builder_log();
        LOG_TRACE_L1(log, "[{func}] Building AnalysisSystem from SSP", __func__);

        auto analysis_sys = std::make_unique<AnalysisSystem>(ssp->ssd->System, fmu_handler);
        // Apply SSP parameter overrides
        auto overrides = ext::ssp1::ssv::apply_parameter_mappings(ssp->ssd->System);
        int override_count = 0;
        for (auto *model : analysis_sys->get_all_models())
        {
            for (auto &connector : model->connectors)
            {
                auto key = AnalysisConnector::get_connector_name(
                    connector->component_name, connector->connector_name);
                auto override_val = override_start_value(overrides, key);
                if (override_val)
                {
                    connector->initial_value = std::move(override_val);
                    ++override_count;
                }
            }
        }
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