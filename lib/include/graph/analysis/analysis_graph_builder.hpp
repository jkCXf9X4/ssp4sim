#pragma once

#include "fmu_handler.hpp"

#include "analysis_graph.hpp"

#include <map>
#include <memory>
#include <string>

namespace ssp4sim::analysis::graph
{

    class AnalysisGraphBuilder
    {
    public:
        ssp4cpp::utils::log::Logger* log = nullptr;

        ssp4cpp::Ssp *ssp;
        handler::FmuHandler *fmu_handler;

        AnalysisGraphBuilder(ssp4cpp::Ssp *ssp, handler::FmuHandler *fmu_handler);

        std::unique_ptr<AnalysisGraph> build();

    private:
        void attach_connectors_to_models(std::map<std::string, std::unique_ptr<AnalysisConnector>> &connectors, std::map<std::string, std::unique_ptr<AnalysisModel>> &models);
        void wire_connections(std::map<std::string, std::unique_ptr<AnalysisConnection>> &connections,
                              std::map<std::string, std::unique_ptr<AnalysisModel>> &models,
                              std::map<std::string, std::unique_ptr<AnalysisConnector>> &connectors);

        void wire_internal_dependencies(std::map<std::string, std::unique_ptr<AnalysisModelVariable>> &model_variables,
                                        std::map<std::string, std::unique_ptr<AnalysisConnector>> &connectors);
        void compute_feedthrough(std::map<std::string, std::unique_ptr<AnalysisConnector>> &connectors,
                                std::map<std::string, std::unique_ptr<AnalysisModel>> &models);
    };

}
