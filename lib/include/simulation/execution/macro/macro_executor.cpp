#include "macro/macro_executor.hpp"
#include "config.hpp"

namespace ssp4sim::graph
{

    MacroExecutor::MacroExecutor(std::vector<std::unique_ptr<Invocable>> nodes) : ExecutionBase(std::move(nodes), "ssp4sim.graph.MacroExecutor")
    {
        macro_step = utils::time::s_to_ns(utils::Config::getDouble("simulation.timestep"));
    }

    uint64_t MacroExecutor::invoke(StepData step_data)
    {
        
        IF_LOG({
            LOG_DEBUG(log, "[{func}] Invoking RealtimeMacroExecutor, full step: {step}", __func__, step_data.to_string());
        });
        
        auto step_start = step_data.start_time;
        while (step_start < step_data.end_time)
        {

            auto step_end =  step_start + macro_step;
            auto s = StepData(step_start, step_end);

            IF_LOG({
                LOG_TRACE_L1(log, "[{func}] RealtimeMacroExecutor executing step: {}", __func__, s.to_string());
            });

            // should only be one node, fallback is to run them in sequent
            for (auto &node : nodes)
            {
                node->invoke(s);
            }

            step_start = step_end;
        }

        return step_start;
    }

}
