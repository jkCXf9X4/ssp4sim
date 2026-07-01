#include "analysis/analysis_system_builder.hpp"

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

        ssp4cpp::utils::log::Logger *log()
        {
            static ssp4cpp::utils::log::Logger *logger =
                ssp4cpp::utils::log::make_logger("ssp4sim.analysis.SspSystemBuilder");
            return logger;
        }
    }


    SspSystem SspSystemBuilder::build(ssp4cpp::Ssp *ssp)
    {
        LOG_TRACE_L1(log(), "[{func}] Building SspSystem from SSP", __func__);

        analysis_system = SspSystem(ssp->ssd->System);
        
        LOG_TRACE_L1(log(), "[{func}] exit", __func__);
        return analysis_system;
    }

} // namespace ssp4sim::analysis
