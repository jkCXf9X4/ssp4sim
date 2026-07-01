#pragma once

#include "shared_config.hpp"
#include "graph_executor.hpp"

#include "pre/ssp_graph_data.hpp"
#include "utils/fmi/fmu_info.hpp"

#include <map>
#include <memory>
#include <string>

namespace ssp4sim::graph
{

    class GraphBuilder
    {
    public:
        ssp4cpp::utils::log::Logger *log = nullptr;

        ssp4sim::signal::DataRecorder *recorder;
        ssp4sim::SharedConfig *config;

        std::map<std::string, std::unique_ptr<Invocable>> models;

        GraphBuilder(ssp4sim::signal::DataRecorder *recorder,
                     ssp4sim::SharedConfig *config);

        void build(analysis::AnalysisGraphData *graph_data);

        std::unique_ptr<Graph> get_graph();

        std::map<std::string, std::unique_ptr<Invocable>> get_models();

    private:
        /// Owns the FmuInfo objects created during build().
        //TODO: move the ownage of the FmuInfo to the model_fmu
        std::vector<std::unique_ptr<handler::FmuInfo>> fmu_infos;

        void create_fmu_models(analysis::AnalysisGraphData &graph_data);
        void create_data_storage_areas(analysis::AnalysisGraphData &graph_data);
        void wire_connections(analysis::AnalysisGraphData &graph_data);
        void derive_model_edges(analysis::AnalysisGraphData &graph_data);
    };

} // namespace ssp4sim::graph