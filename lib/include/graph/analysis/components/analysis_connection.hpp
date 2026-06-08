// DEPRECATED: Use lib/include/analysis/analysis_connection.hpp instead.
// This file is kept for backward compatibility. New code should include
// "analysis/analysis_connection.hpp" from namespace ssp4sim::analysis.
#pragma once

#include "utils/node.hpp"

#include "ssp4cpp/schema/ssp1/SSP1_SystemStructureDescription.hpp"
#include "ssp4cpp/utils/log.hpp"

#include "ssp4cpp/ssp.hpp"

#include <cstdint>
#include <map>
#include <string>

namespace ssp4sim::analysis::graph
{

    class AnalysisConnector;
    class AnalysisModel;

    class AnalysisConnection : public ssp4sim::utils::graph::Node
    {
    public:
        uint64_t delay = 0;

        ssp4cpp::utils::log::Logger* log = nullptr;
        std::string source_component_name;
        std::string source_connector_name;
        std::string target_component_name;
        std::string target_connector_name;

        AnalysisConnector *source_connector;
        AnalysisConnector *target_connector;

        AnalysisModel *source_model;
        AnalysisModel *target_model;

        AnalysisConnection();

        AnalysisConnection(ssp4cpp::ssp1::ssd::Connection *connection);

        ~AnalysisConnection();

        void update_name();

        static std::string create_name(const std::string &start_com,
                                       const std::string &start_con,
                                       const std::string &end_com,
                                       const std::string &end_con);

        std::string get_source_connector_name() const;

        std::string get_target_connector_name() const;

        std::string to_string() const override;
    };

    std::map<std::string, std::unique_ptr<AnalysisConnection>> create_connections(ssp4cpp::Ssp &ssp_ref,  ssp4cpp::utils::log::Logger *log);

}
