#pragma once

#include "ssp4sim_definitions.hpp"

#include "executor_base.hpp"

#include <cstdint>
#include <memory>
#include <vector>


namespace ssp4sim::graph
{

    /// @brief Sweeps nodes over the caller's span in fixed `macro_step` slices.
    ///
    /// invoke() divides [start, end) into `macro_step`-sized slices and runs
    /// the wrapped nodes once per slice (typically a single composed executor:
    /// the specialized stack built by ExecutorBuilder). The final slice is
    /// clamped at `end`, so the union of the emitted slices always exactly
    /// equals [start, end].
    ///
    /// `realtime` (default false) paces every slice to the wall clock via
    /// ExecutorBase::wait_for_realtime_sync.
    class MacroExecutor final : public ExecutorBase
    {
    public:
        /// `macro_step` (ns) is injected by ExecutorBuilder from
        /// `simulation.timestep`, so this executor stays free of the global
        /// Config.
        MacroExecutor(std::vector<std::shared_ptr<Invocable>> nodes,
                      uint64_t macro_step,
                      const bool realtime = false);

        uint64_t invoke(StepData step_data) override;

        uint64_t macro_step = 0;
    };

}

