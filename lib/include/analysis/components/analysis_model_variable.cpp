#include "analysis/components/analysis_model_variable.hpp"
#include "analysis/components/analysis_connector.hpp"

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
                ssp4cpp::utils::log::make_logger("ssp4sim.analysis.AnalysisModelVariable");
            return logger;
        }
    }

    AnalysisModelVariable::AnalysisModelVariable(const ssp4cpp::fmi2::md::fmi2ScalarVariable &var)
        : name(var.name)
    {
        component = "";
        variable_name = var.name;
        type = "";
        if (var.valueReference.has_value())
        {
            value_reference = var.valueReference.value();
        }
    }

    AnalysisModelVariable::AnalysisModelVariable(std::string component_,
                                                  std::string var_name,
                                                  std::string type_,
                                                  std::string /*initial_value*/)
        : component(std::move(component_)),
          variable_name(var_name),
          type(std::move(type_)),
          name(AnalysisConnector::get_connector_name(this->component, this->variable_name))
    {
    }

    std::string AnalysisModelVariable::to_string() const
    {
        std::ostringstream oss;
        oss << "AnalysisModelVariable {"
            << "\n  name: " << name
            << "\n  component: " << component
            << "\n  variable_name: " << variable_name
            << "\n  type: " << type
            << "\n  vr: " << value_reference
            << "\n  causality: " << causality.to_string()
            << "\n}";
        return oss.str();
    }

} // namespace ssp4sim::analysis