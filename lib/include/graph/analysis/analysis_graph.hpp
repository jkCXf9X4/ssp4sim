#pragma once

#include "ssp4sim_definitions.hpp"

#include "analysis_connection.hpp"
#include "analysis_model.hpp"
#include "analysis_connector.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ssp4sim::analysis::graph
{
    class AnalysisGraph : public types::IWritable
    {
    public:
        Logger log = Logger("ssp4sim.graph.AnalysisGraph", LogLevel::debug);

        std::map<std::string, std::unique_ptr<AnalysisModel>> models;
        std::map<std::string, std::unique_ptr<AnalysisConnector>> connectors;
        std::map<std::string, std::unique_ptr<AnalysisConnection>> connections;

        std::vector<AnalysisModel *> nodes;

        AnalysisGraph() = default;

        AnalysisGraph(std::map<std::string, std::unique_ptr<AnalysisModel>> models_,
                      std::map<std::string, std::unique_ptr<AnalysisConnector>> connectors_,
                      std::map<std::string, std::unique_ptr<AnalysisConnection>> connections_);

        std::vector<AnalysisModel *> get_start_nodes() const;

        std::string to_string() const override;

    };

}
