// DEPRECATED: Use lib/include/analysis/analysis_connection.hpp instead.
// This file is kept for backward compatibility.
#include "graph/analysis/components/analysis_connection.hpp"

#include "graph/analysis/components/analysis_connector.hpp"

#include "utils/time.hpp"

#include <sstream>

namespace ssp4sim::analysis::graph
{

    AnalysisConnection::AnalysisConnection()
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.graph.AnalysisConnection"))
    {
    }

    AnalysisConnection::AnalysisConnection(ssp4cpp::ssp1::ssd::Connection *connection)
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.graph.AnalysisConnection"))
    {
        if (!connection->startElement.has_value() || !connection->endElement.has_value())
        {
            throw std::runtime_error("System-boundary connections are not analysis connections");
        }

        source_component_name = connection->startElement.value();
        source_connector_name = connection->startConnector;
        target_component_name = connection->endElement.value();
        target_connector_name = connection->endConnector;

        update_name();
    }

    AnalysisConnection::~AnalysisConnection()
    {
        LOG_TRACE_L1(log, "[{func}] Destroying AnalysisConnection", __func__);
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

    std::string AnalysisConnection::to_string() const
    {
        std::ostringstream oss;
        oss << "Connection {"
            << "\nname: " << name
            << "\nsource_component_name: " << source_component_name
            << "\nsource_connector_name: " << source_connector_name
            << "\ntarget_component_name: " << target_component_name
            << "\ntarget_connector_name: " << target_connector_name
            << "\n }\n";
        return oss.str();
    }

    std::map<std::string, std::unique_ptr<AnalysisConnection>> create_connections(ssp4cpp::Ssp &ssp_ref,  ssp4cpp::utils::log::Logger *log)
    {
        LOG_TRACE_L1(log, "[{func}] init", __func__);
        std::map<std::string, std::unique_ptr<AnalysisConnection>> items;
        if (ssp_ref.ssd->System.Connections.has_value())
        {
            for (auto &connection : ssp_ref.ssd->System.Connections.value().Connections)
            {
                // System boundary connections, pass over for now
                if (!connection.startElement.has_value() || !connection.endElement.has_value())
                {
                    LOG_WARNING_LIMIT_EVERY_N(100000, log, "[{func}] System level connections are not supported as of now", __func__);
                    continue;
                }
                auto c = std::make_unique<AnalysisConnection>(&connection);
                LOG_TRACE_L1(log, "[{func}] New Connection: {connection}", __func__, c->name);
                c->delay = utils::time::s_to_ns(connection.information_delay.value_or(0));
                items[c->name] = std::move(c);
            }
        }
        LOG_DEBUG(log, "[{func}] exit, Total connections created: {count}", __func__, items.size());
        return items;
    }

}
