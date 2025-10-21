#pragma once

#include "utils/node.hpp"
#include "utils/map.hpp"
#include "utils/string.hpp"
#include "utils/time.hpp"
#include "utils/log.hpp"

#include "analysis_connector.hpp"
#include "analysis_model.hpp"

#include <string>
#include <vector>

namespace ssp4sim::sim::analysis::graph
{

    class AnalysisConnection : public ssp4sim::utils::graph::Node
    {
        uint64_t delay = 0;

    public:
        common::Logger log = common::Logger("ssp4sim.graph.AnalysisConnection", common::LogLevel::debug);
        std::string source_component_name;
        std::string source_connector_name;
        std::string target_component_name;
        std::string target_connector_name;

        AnalysisConnector *source_connector;
        AnalysisConnector *target_connector;

        AnalysisModel *source_model;
        AnalysisModel *target_model;

        AnalysisConnection()
        {
        }

        AnalysisConnection(ssp4sim::ssp1::ssd::Connection *connection)
        {
            source_component_name = connection->startElement.value();
            source_connector_name = connection->startConnector;
            target_component_name = connection->endElement.value();
            target_connector_name = connection->endConnector;

            update_name();
        }

        ~AnalysisConnection()
        {
            log.ext_trace("[{}] Destroying AnalysisConnection", __func__);
        }

        void update_name()
        {
            this->name = AnalysisConnection::create_name(source_component_name, source_connector_name, target_component_name, target_connector_name);
        }

        static std::string create_name(std::string &start_com, std::string &start_con, std::string &end_com, std::string &end_con)
        {
            return start_com + "." + start_con + "->" + end_com + "." + end_con;
        }

        std::string get_source_connector_name()
        {
            return AnalysisConnector::create_name(source_component_name, source_connector_name);
        }

        std::string get_target_connector_name()
        {
            return AnalysisConnector::create_name(target_component_name, target_connector_name);
        }

        virtual void print(std::ostream &os) const
        {
            os << "Connection {"
               << "\nname: " << name
               << "\nsource_component_name: " << source_component_name
               << "\nsource_connector_name: " << source_connector_name
               << "\ntarget_component_name: " << target_component_name
               << "\ntarget_connector_name: " << target_connector_name
               << "\n }\n";

        }

    };

}