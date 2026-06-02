#include "graph/analysis/components/analysis_internal.hpp"

#include "graph/analysis/components/analysis_connector.hpp"

#include <sstream>

// #include <utility>

namespace ssp4sim::analysis::graph
{

    AnalysisModelVariable::AnalysisModelVariable()
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.graph.AnalysisModelVariable"))
    {
    }

    AnalysisModelVariable::AnalysisModelVariable(std::string component, std::string variable_name)
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.graph.AnalysisModelVariable"))
    {
        this->component = component;
        this->variable_name = variable_name;
        update_name();
    }

    AnalysisModelVariable::~AnalysisModelVariable()
    {
        LOG_TRACE_L1(log, "[{func}] Destroying AnalysisModelVariable", __func__);
    }

    void AnalysisModelVariable::update_name()
    {
        this->name = AnalysisConnector::create_name(component, variable_name);
    }

    std::string AnalysisModelVariable::get_connector_name() const
    {
        return AnalysisConnector::create_name(component, variable_name);
    }

    std::string AnalysisModelVariable::to_string() const
    {
        std::ostringstream oss;
        oss << "AnalysisModelVariable {"
            << "\nname: " << name
            << "\ncomponent: " << component
            << "\nvariable_name: " << variable_name
            << "\n}\n";
        return oss.str();
    }

}
