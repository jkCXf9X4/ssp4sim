#include "analysis/components/analysis_parameter_bindings.hpp"

#include "SSP1_SystemStructureParameter_Ext.hpp"


#include "ssp4cpp/utils/log.hpp"

#include <sstream>
#include <utility>

namespace ssp4sim::analysis
{
    namespace
    {
        ssp4cpp::utils::log::Logger *log()
        {
            static ssp4cpp::utils::log::Logger *logger =
                ssp4cpp::utils::log::make_logger("ssp4sim.analysis.AnalysisConnection");
            return logger;
        }
    }

    AnalysisParameterBindings::AnalysisParameterBindings(std::optional<ssp4cpp::ssp1::ssd::TParameterBindings> bindings, ssp4cpp::Ssp *ssp)
    {
        if (bindings.has_value())
        {
            parameter_bindings = ext::ssp1::ssv::get_start_value_mappings(
                bindings->ParameterBindings, ssp);
        }
    }

    std::string AnalysisParameterBindings::to_string() const
    {
        std::ostringstream oss;
        oss << "AnalysisParameterBindings {"
            << "\n}";
        return oss.str();
    }

} // namespace ssp4sim::analysis