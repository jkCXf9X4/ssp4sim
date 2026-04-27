#include "graph/analysis/analysis_connector.hpp"

#include "FMI2_Enums_Ext.hpp"
#include "SSP1_SystemStructureParameter_Ext.hpp"

#include <sstream>

namespace ssp4sim::analysis::graph
{

    AnalysisConnector::AnalysisConnector()
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.graph.AnalysisConnector"))
    {
    }

    AnalysisConnector::AnalysisConnector(std::string component_name,
                                         std::string connector_name,
                                         uint64_t value_reference,
                                         types::DataType type)
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.graph.AnalysisConnector"))
    {
        this->component_name = component_name;
        this->connector_name = connector_name;
        update_name();

        this->value_reference = value_reference;
        this->type = type;
        this->size = ssp4sim::ext::fmi2::enums::get_data_type_size(type);
    }

    AnalysisConnector::~AnalysisConnector()
    {
        LOG_TRACE_L1(log, "[{func}] Destroying AnalysisConnector", __func__);
    }

    void AnalysisConnector::update_name()
    {
        this->name = AnalysisConnector::create_name(component_name, connector_name);
    }

    std::string AnalysisConnector::create_name(const std::string &component_name, const std::string &connector_name)
    {
        return component_name + "." + connector_name;
    }

    std::string AnalysisConnector::to_string() const
    {
        std::ostringstream oss;
        oss << "Connector {"
            << "\nname: " << name
            << "\nvr: " << value_reference
            << "\ntype: " << type
            << "\ncausality: " << causality
            << "\n }\n";
        return oss.str();
    }

}
