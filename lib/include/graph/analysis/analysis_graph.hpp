#pragma once

#include "ssp4sim_definitions.hpp"

#include "graph/analysis/components/analysis_connection.hpp"
#include "graph/analysis/components/analysis_model.hpp"
#include "graph/analysis/components/analysis_connector.hpp"
#include "graph/analysis/components/analysis_internal.hpp"

#include "utils/node.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ssp4sim::analysis::graph
{
    class AnalysisGraph : public types::IWritable
    {
    public:
        ssp4cpp::utils::log::Logger* log = nullptr;

        std::map<std::string, std::unique_ptr<AnalysisModel>> models;
        std::map<std::string, std::unique_ptr<AnalysisConnector>> connectors;
        std::map<std::string, std::unique_ptr<AnalysisConnection>> connections;
        std::map<std::string, std::unique_ptr<AnalysisModelVariable>> model_variables;

        std::vector<AnalysisModel *> nodes;

        AnalysisGraph() = default;

        AnalysisGraph(std::map<std::string, std::unique_ptr<AnalysisModel>> models_,
                      std::map<std::string, std::unique_ptr<AnalysisConnector>> connectors_,
                      std::map<std::string, std::unique_ptr<AnalysisConnection>> connections_,
                      std::map<std::string, std::unique_ptr<AnalysisModelVariable>> model_variables_);

        std::vector<AnalysisModel *> get_start_nodes() const;

        std::vector<ssp4sim::utils::graph::Node *> get_nodes() const;

        std::vector<std::vector<utils::graph::Node *>> strongly_connected_components() const;

        std::string to_string() const override;

    };

}
