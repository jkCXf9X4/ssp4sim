#pragma once

#include "ssp4cpp/ssp.hpp"
#include "utils/primitives/map.hpp"

#include <map>
#include <memory>
#include <string>

namespace ssp4sim::graph
{
    class Invocable;
}

namespace ssp4sim
{
    struct SharedConfig;
}

namespace ssp4sim::pre
{

    /// Result of the pre-simulation pipeline.
    struct SimulationGraph
    {
        std::map<std::string, std::unique_ptr<graph::Invocable>> models;
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
    SimulationGraph build_simulation_graph(
        ssp4cpp::Ssp *ssp,
        ssp4sim::SharedConfig *config);

} // namespace ssp4sim::pre