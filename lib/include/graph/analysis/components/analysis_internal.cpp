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


    std::map<std::string, std::unique_ptr<AnalysisModelVariable>>
    create_model_variables(std::map<std::string, ssp4cpp::Fmu *> &fmu_map, ssp4cpp::utils::log::Logger *log)
    {
        LOG_TRACE_L1(log, "[{func}] init", __func__);
        std::map<std::string, std::unique_ptr<AnalysisModelVariable>> items;
        for (auto &[name, fmu] : fmu_map)
        {
            for (auto &variable : fmu->md->ModelVariables.ScalarVariable)
            {
                auto mv = std::make_unique<AnalysisModelVariable>(name, variable.name);
                LOG_TRACE_L1(log, "[{func}] New ModelVariable: {variable}", __func__, mv->name);
                items[mv->name] = std::move(mv);
            }
        }
        LOG_TRACE_L1(log, "[{func}] exit, Total model variables created: {count}", __func__, items.size());
        return items;
    }

}
