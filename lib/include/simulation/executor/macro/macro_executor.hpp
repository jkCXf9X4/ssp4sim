#pragma once

#include "ssp4sim_definitions.hpp"

#include "executor_base.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>


namespace ssp4sim::graph
{

    class MacroExecutor final : public ExecutorBase
    {
    public:
        /// `macro_step` (ns) is injected by ExecutorBuilder from
        /// `simulation.timestep`, so this executor stays free of the global
        /// Config.
        MacroExecutor(std::vector<std::shared_ptr<Invocable>> nodes,
                      uint64_t macro_step);

        uint64_t invoke(StepData step_data) override;

        uint64_t macro_step = 0;
    };

}

