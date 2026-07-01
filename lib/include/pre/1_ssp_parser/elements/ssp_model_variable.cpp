#include "analysis/components/analysis_model_variable.hpp"

#include "FMI2_modelDescription_Ext.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <sstream>
#include <utility>

namespace ssp4sim::analysis
{
    namespace
    {
        ssp4cpp::utils::log::Logger *model_log()
        {
            static ssp4cpp::utils::log::Logger *logger =
                ssp4cpp::utils::log::make_logger("ssp4sim.analysis.SspModelVariable");
            return logger;
        }
    }

    SspModelVariable::SspModelVariable(const ssp4cpp::fmi2::md::fmi2ScalarVariable &var)
    {
        name = var.name;
        type = SspItemType::ModelVariable;
        if (var.valueReference.has_value())
        {
            value_reference = var.valueReference.value();
        }
    }

    std::string SspModelVariable::to_string() const
    {
        std::ostringstream oss;
        oss << "SspModelVariable {"
            << "\n  name: " << name
            << "\n  vr: " << value_reference
            << "\n  causality: " << causality.to_string()
            << "\n}";
        return oss.str();
    }

} // namespace ssp4sim::analysis