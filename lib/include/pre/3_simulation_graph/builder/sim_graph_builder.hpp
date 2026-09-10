#pragma once

#include "pre/2_analysis_graph/ssp_graph_data.hpp"
#include "utils/fmi/fmu_info.hpp"
#include "pre/3_simulation_graph/elements/invocable.hpp"

#include <map>
#include <memory>
#include <string>

namespace ssp4sim::signal
{
    class DataRecorder;
}

namespace ssp4sim::graph
{

    class FmuModel;

    class GraphBuilder
    {
    public:
        bool record_inputs;

        explicit GraphBuilder(bool record_inputs);

        std::map<std::string, std::shared_ptr<Invocable>> build(analysis::AnalysisGraphData *graph_data);

        /// Register every model's value storages with the recorder (input area when
        /// record_inputs is set, output area always). No-op for a null recorder and for
        /// non-FmuModel entries.
        static void register_model_storages(
            const std::map<std::string, std::shared_ptr<Invocable>> &models,
            ssp4sim::signal::DataRecorder *recorder);

    private:
        ssp4cpp::utils::log::Logger *log = nullptr;

        std::map<std::string, std::shared_ptr<Invocable>> models;

        void create_fmu_models(analysis::AnalysisGraphData &graph_data);
        void create_data_storage_areas(analysis::AnalysisGraphData &graph_data);
        void wire_connections(analysis::AnalysisGraphData &graph_data);
        void derive_model_edges(analysis::AnalysisGraphData &graph_data);

        void register_connector_storage(FmuModel* model, analysis::SspConnectorNode* conn_node);
        void wire_connection(FmuModel* source_model, analysis::SspConnectorNode* conn_node);

        FmuModel* as_fmu(Invocable* invocable);

        static analysis::SspModelNode* find_peer_model_node(analysis::SspConnectorNode* conn_node);

        static analysis::SspModelNode* trace_boundary_connectors(
            analysis::SspConnectorNode* peer_conn_node,
            analysis::SspConnectorNode*& resolved_peer,
            analysis::SspConnector*& resolved_connector);
    };

} // namespace ssp4sim::graph