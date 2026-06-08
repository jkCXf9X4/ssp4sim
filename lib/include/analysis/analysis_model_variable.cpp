#include "analysis/analysis_model_variable.hpp"

#include <sstream>
#include <utility>

namespace ssp4sim::analysis
{

    AnalysisModelVariable::AnalysisModelVariable(std::string component_,
                                                  std::string variable_name_,
                                                  std::string type_,
                                                  std::string value_reference_)
        : component(std::move(component_)),
          variable_name(std::move(variable_name_)),
          type(std::move(type_)),
          value_reference(std::move(value_reference_))
    {
        name = component + "." + variable_name;
    }

    std::string AnalysisModelVariable::to_string() const
    {
        std::ostringstream oss;
        oss << "AnalysisModelVariable {"
            << "\n  name: " << name
            << "\n  component: " << component
            << "\n  variable_name: " << variable_name
            << "\n  type: " << type
            << "\n  causality: " << causality
            << "\n}";
        return oss.str();
    }

} // namespace ssp4sim::analysis