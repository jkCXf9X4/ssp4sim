#include "graph/analysis/analysis_connection.hpp"

#include "graph/analysis/analysis_connector.hpp"

namespace ssp4sim::analysis::graph
{

    AnalysisConnection::AnalysisConnection(ssp4cpp::ssp1::ssd::Connection *connection)
    {
        source_component_name = connection->startElement.value();
        source_connector_name = connection->startConnector;
        target_component_name = connection->endElement.value();
        target_connector_name = connection->endConnector;

        update_name();
    }

    AnalysisConnection::~AnalysisConnection()
    {
        log(ext_trace)("[{}] Destroying AnalysisConnection", __func__);
    }

    void AnalysisConnection::update_name()
    {
        this->name = AnalysisConnection::create_name(source_component_name, source_connector_name, target_component_name, target_connector_name);
    }

    std::string AnalysisConnection::create_name(const std::string &start_com,
                                                const std::string &start_con,
                                                const std::string &end_com,
                                                const std::string &end_con)
    {
        return start_com + "." + start_con + "->" + end_com + "." + end_con;
    }

    std::string AnalysisConnection::get_source_connector_name() const
    {
        return AnalysisConnector::create_name(source_component_name, source_connector_name);
    }

    std::string AnalysisConnection::get_target_connector_name() const
    {
        return AnalysisConnector::create_name(target_component_name, target_connector_name);
    }

    void AnalysisConnection::print(std::ostream &os) const
    {
        os << "Connection {"
           << "\nname: " << name
           << "\nsource_component_name: " << source_component_name
           << "\nsource_connector_name: " << source_connector_name
           << "\ntarget_component_name: " << target_component_name
           << "\ntarget_connector_name: " << target_connector_name
           << "\n }\n";
    }

}
