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

namespace ssp4sim
{
    struct SharedConfig;
}

namespace ssp4sim::pre
{

    /// Result of the pre-simulation setup: the executable simulation node (an
    /// executor wrapping the models, with the read-path resolver wired in).
    struct SimulationData
    {
        std::shared_ptr<graph::ExecutionBase> execution_node;
    };

    /// Build the executable simulation graph from the pre-built models.
    ///
    /// Registers the model value storages with the recorder (if any), then builds
    /// the configured executor over the models. The executor wires the shared
    /// DataAccessResolver into every FmuModel (see ExecutorBuilder::build).
    SimulationData setup_sim_behaviour(
        std::map<std::string, std::shared_ptr<graph::Invocable>> models,
        signal::DataRecorder *recorder,
        ssp4sim::SharedConfig *config);

} // namespace ssp4sim::pre