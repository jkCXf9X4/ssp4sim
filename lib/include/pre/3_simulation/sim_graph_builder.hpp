#pragma once

#include "pre/ssp_graph_data.hpp"
#include "utils/fmi/fmu_info.hpp"
#include "execution/invocable.hpp"

#include <map>
#include <memory>
#include <string>

namespace ssp4sim::signal
{
    class DataRecorder;
}

namespace ssp4sim::graph
{

    class GraphBuilder
    {
    public:
        ssp4cpp::utils::log::Logger *log = nullptr;

        bool record_inputs;

        std::map<std::string, std::unique_ptr<Invocable>> models;

        explicit GraphBuilder(bool record_inputs);

        std::map<std::string, std::unique_ptr<Invocable>> build(analysis::AnalysisGraphData *graph_data);

    private:
        void create_fmu_models(analysis::AnalysisGraphData &graph_data);
        void create_data_storage_areas(analysis::AnalysisGraphData &graph_data);
        void wire_connections(analysis::AnalysisGraphData &graph_data);
        void derive_model_edges(analysis::AnalysisGraphData &graph_data);
    };

    void register_model_storages(
        const std::map<std::string, std::unique_ptr<Invocable>> &models,
        ssp4sim::signal::DataRecorder *recorder);

} // namespace ssp4sim::graph