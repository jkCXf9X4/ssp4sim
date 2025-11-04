#include "graph/analysis/analysis_internal.hpp"

#include "graph/analysis/analysis_connector.hpp"

#include <utility>

namespace ssp4sim::analysis::graph
{

    AnalysisModelVariable::AnalysisModelVariable() = default;

    AnalysisModelVariable::AnalysisModelVariable(std::string component, std::string variable_name)
    {
        this->component = component;
        this->variable_name = variable_name;
        update_name();
    }

    AnalysisModelVariable::~AnalysisModelVariable()
    {
        log(ext_trace)("[{}] Destroying AnalysisModelVariable", __func__);
    }

    void AnalysisModelVariable::update_name()
    {
        this->name = AnalysisConnector::create_name(component, variable_name);
    }

    std::string AnalysisModelVariable::get_connector_name() const
    {
        return AnalysisConnector::create_name(component, variable_name);
    }

    void AnalysisModelVariable::print(std::ostream &os) const
    {
        os << "AnalysisModelVariable {"
           << "\nname: " << name
           << "\ncomponent: " << component
           << "\nvariable_name: " << variable_name
           << "\n}\n";
    }

}
