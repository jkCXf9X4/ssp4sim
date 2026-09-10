#pragma once

#include "utils/primitives/map.hpp"

#include <map>
#include <memory>
#include <string>

namespace ssp4sim::signal
{
    class DataRecorder;
}

namespace ssp4sim::graph
{
    class Invocable;
    class ExecutionBase;
}

namespace ssp4sim::scheduling
{
    class ReadTargetResolver;
}

namespace ssp4sim
{
    struct SharedConfig;
}

namespace ssp4sim::pre
{

    /// Result of the pre-simulation pipeline.
    struct SimulationData
    {
        std::shared_ptr<ExecutionBase> execution_node;
    };

    /// Build the simulation models from an SSP.
    ///
    /// Encapsulates the four-stage pipeline:
    ///   1. Build analysis system from SSP
    ///   2. Build analysis tree
    ///   3. Build analysis graph
    ///   4. Build simulation models (FmuModel with connectors, wiring, edges)
    ///
    /// Returns the simulation models. The caller constructs the GraphExecutor
    /// from these models.
    SimulationData setup_sim_behaviour(
        std::map<std::string, std::shared_ptr<graph::Invocable>> models, signal::DataRecorder *recorder, ssp4sim::SharedConfig *config)
    {
        register_model_storages(models, recorder);

        LOG_INFO(p->log, "[{func}] - Creating simulation graph executor", __func__);

        p->sim_graph = executor_builder(p->setup_results.get_models());

        LOG_DEBUG(p->log, " -- {graph}", p->sim_graph->to_string());
    }

    static void register_model_storages(
        const std::map<std::string, std::shared_ptr<graph::Invocable>> &models,
        ssp4sim::signal::DataRecorder *recorder)
    {
        if (!recorder)
            return;

        for (auto &[name, model] : models)
        {
            auto m = dynamic_cast<FmuModel *>(model.get());
            if (!m)
                continue;

            if (m->record_inputs)
            {
                recorder->add_storage(m->input_area.get());
            }
            recorder->add_storage(m->output_area.get());
        }
    }

} // namespace ssp4sim::pre