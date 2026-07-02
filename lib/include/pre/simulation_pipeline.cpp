#include "simulation_pipeline.hpp"

#include "pre/1_ssp_parser/ssp_parser.hpp"
#include "pre/2_analysis/tree_builder.hpp"
#include "pre/2_analysis/graph_builder.hpp"
#include "pre/3_simulation/sim_graph_builder.hpp"

#include "config.hpp"
#include "execution/invocable.hpp"
#include "signal/recorder.hpp"

#include "ssp4cpp/utils/log.hpp"

namespace ssp4sim::pre
{

    SimulationPipelineResult build_simulation_graph(
        ssp4cpp::Ssp *ssp,
        bool set_record_inputs)
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

        LOG_INFO(log, "[{func}] - Creating simulation models", __func__);
        auto sim_graph_builder = graph::GraphBuilder(set_record_inputs);

        SimulationPipelineResult result;
        result.models = sim_graph_builder.build(&analysis_graph_data);

                LOG_INFO(log, "[{func}] - Pipeline complete, {} models built", __func__, result.models.size());
        return result;
    }

} // namespace ssp4sim::pre