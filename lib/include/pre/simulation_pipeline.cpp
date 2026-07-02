#include "simulation_pipeline.hpp"

#include "pre/1_ssp_parser/ssp_parser.hpp"
#include "pre/2_analysis/tree_builder.hpp"
#include "pre/2_analysis/graph_builder.hpp"
#include "pre/3_simulation/sim_graph_builder.hpp"

#include "config.hpp"
#include "execution/invocable.hpp"
#include "signal/recorder.hpp"
#include "shared_config.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <cstdlib>
#include <fstream>

namespace ssp4sim::pre
{

    SimulationPipelineResult build_simulation_graph(
        ssp4cpp::Ssp *ssp,
        ssp4sim::SharedConfig *config)
    {
        auto log = ssp4cpp::utils::log::make_logger("ssp4sim.pre.SimulationPipeline");

        LOG_INFO(log, "[{func}] - Creating analysis system", __func__);
        auto analysis_system = analysis::SspSystemBuilder().build(ssp);
        LOG_DEBUG(log, " -- analysis system built");

        LOG_INFO(log, "[{func}] - Building analysis tree", __func__);
        analysis::SspTreeBuilder tree_builder;
        auto *system_tree = tree_builder.build(&analysis_system);
        LOG_DEBUG(log, " -- analysis tree built");

        LOG_INFO(log, "[{func}] - Building analysis graph", __func__);
        analysis::SspGraphBuilder graph_builder;
        auto analysis_graph_data = graph_builder.build(system_tree);
        LOG_DEBUG(log, " -- analysis graph built");

        {
            LOG_DEBUG(log, " -- writing debug output files");
            std::ofstream(config->working_dir / "tree_output.txt") << system_tree->get_tree();

            std::vector<utils::graph::Node *> all_nodes;
            for (auto &n : analysis_graph_data.model_nodes)
                all_nodes.push_back(n.get());
            for (auto &n : analysis_graph_data.connector_nodes)
                all_nodes.push_back(n.get());
            for (auto &n : analysis_graph_data.connection_nodes)
                all_nodes.push_back(n.get());
            std::ofstream(config->working_dir / "graph_output.dot") << utils::graph::Node::to_dot(all_nodes);
        }

        LOG_INFO(log, "[{func}] - Creating simulation models", __func__);
        auto sim_graph_builder = graph::GraphBuilder(config->record_inputs);

        SimulationPipelineResult result;
        result.models = sim_graph_builder.build(&analysis_graph_data);

        LOG_INFO(log, "[{func}] - Pipeline complete, {} models built", __func__, result.models.size());
        return result;
    }

} // namespace ssp4sim::pre