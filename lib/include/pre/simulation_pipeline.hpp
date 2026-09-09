#pragma once

#include "ssp4cpp/ssp.hpp"
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
    struct SimulationPipelineResult
    {
        std::map<std::string, std::unique_ptr<graph::Invocable>> models;

        /// Read-path resolver, owned here so it outlives the models (they borrow it
        /// via FmuModel::access_resolver). Built from the wired models in
        /// build_simulation_graph — the graph-builder stage owns access policy.
        std::unique_ptr<ssp4sim::scheduling::ReadTargetResolver> access_resolver;

        std::map<std::string, graph::Invocable*> get_models()
        {
            return ssp4sim::utils::map_ns::map_unique_to_ref(models);
        }
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
    SimulationPipelineResult build_simulation_graph(
        ssp4cpp::Ssp *ssp,
        ssp4sim::SharedConfig *config);

} // namespace ssp4sim::pre