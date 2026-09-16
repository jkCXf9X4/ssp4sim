#include "executor/substep/macro_substep_executor.hpp"

#include <algorithm>

namespace ssp4sim::graph
{

    MacroExecutor::MacroExecutor(std::vector<std::shared_ptr<Invocable>> nodes,
                                 uint64_t macro_step, const bool realtime)
        : ExecutorBase(std::move(nodes), "ssp4sim.graph.MacroExecutor", realtime),
          macro_step(macro_step)
    {
        this->name = "MacroExecutor";
    }

    uint64_t MacroExecutor::invoke(StepData step_data)
    {
        IF_LOG({
            LOG_DEBUG(log, "[{func}] {name}, full step: {step}", __func__, name, step_data.to_string());
        });

        auto step_start = step_data.start_time;
        while (step_start < step_data.end_time)
        {
            wait_for_realtime_sync(step_start);

            auto step_end = std::min(step_start + macro_step, step_data.end_time);
            auto s = StepData(step_start, step_end);

            IF_LOG({
                LOG_TRACE_L1(log, "[{func}] {name} executing step: {}", __func__, name, s.to_string());
            });

            // should only be one node, fallback is to run them in sequent
            for (auto &node : nodes)
            {
                node->invoke(s);
            }

            step_start = step_end;
        }

        return step_data.end_time;
    }

}