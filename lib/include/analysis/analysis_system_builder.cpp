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

        void override_start_values()
        {
            auto param_mappings = ext::ssp1::ssv::get_start_value_mappings(*ssp);
            // start values
             // Process nested Systems (recursive)
            // start with the lower levels to ensure that higher levels override parameter bindings
            // these should be able to override all levels below

            // start values need to be on a level by level approach
            // if they are applied on a to high level the full path will differ
            // dc motor should be a good example to apply


                // override startvalues with component parameter bindings

                // SSP parameter set overrides supersede FMU-provided start values
                // auto override_iv = override_start_value(parameter_mappings, system_name);
                // if (override_iv)
                //     c->initial_value = std::move(override_iv);



            // apply system level parameter bindings on all levels below
        }
        
    
            


    std::unique_ptr<AnalysisSystem> AnalysisSystemBuilder::build(ssp4cpp::Ssp *ssp,
                                                                 handler::FmuHandler *fmu_handler)
    {
        if (!log)
            log = builder_log();
        LOG_TRACE_L1(log, "[{func}] Building AnalysisSystem from SSP", __func__);

        auto analysis_sys = std::make_unique<AnalysisSystem>(ssp->ssd->System, fmu_handler);

        override_start_values();

        LOG_TRACE_L1(log, "[{func}] exit", __func__);
        return result;
    }

} // namespace ssp4sim::analysis