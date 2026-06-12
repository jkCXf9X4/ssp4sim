#pragma once

#include "shared_config.hpp"
#include "graph.hpp"

#include "analysis/components/analysis_system.hpp"

#include <map>
#include <memory>
#include <string>

namespace ssp4sim::graph
{

    class GraphBuilder
    {
    public:
        ssp4cpp::utils::log::Logger *log = nullptr;

        const analysis::AnalysisSystem &analysis_system;
        ssp4sim::signal::DataRecorder *recorder;
        ssp4sim::SharedConfig *config;

        std::map<std::string, std::unique_ptr<Invocable>> models;

        GraphBuilder(const analysis::AnalysisSystem &analysis_system_,
                     ssp4sim::signal::DataRecorder *recorder,
                     ssp4sim::SharedConfig *config);

        void build();

        std::unique_ptr<Graph> get_graph();

        std::map<std::string, std::unique_ptr<Invocable>> get_models();

    private:
        struct ResolvedConnection {
            std::string source_model;
            std::string source_connector;
            std::string target_model;
            std::string target_connector;
            uint64_t delay;
        };

        void create_fmu_models();
        void create_data_storage_areas();
        void wire_connections();
        void derive_model_edges();

        std::string resolve_model_key(const std::string &name) const;
        std::vector<ResolvedConnection> resolve_connection(
            const analysis::AnalysisConnection *conn) const;
    };

}