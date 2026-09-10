#include "realtime_macro_executor.hpp"

#include "config.hpp"

#include "utils/time/time.hpp"

#include <chrono>
#include <thread>

namespace ssp4sim::graph
{

    RealtimeMacroExecutor::RealtimeMacroExecutor(std::vector<std::shared_ptr<Invocable>> nodes) : ExecutionBase(nodes, "ssp4sim.graph.RealtimeMacroExecutor")
    {
        macro_step = utils::time::s_to_ns(utils::Config::getDouble("simulation.timestep"));
        realtime_start_reference = utils::time::time_now_ns();
    }

    void RealtimeMacroExecutor::wait_for_realtime_sync(uint64_t simulation_time)
    {
        using clock = std::chrono::steady_clock;

        auto target = clock::time_point(std::chrono::nanoseconds(realtime_start_reference + simulation_time));
        std::this_thread::sleep_until(target);

        LOG_INFO(log, "[{func}] Realtime: {time}, TargetTime: {target}", __func__, simulation_time, target.time_since_epoch().count());
    }

    uint64_t RealtimeMacroExecutor::invoke(StepData step_data)
    {
        
        IF_LOG({
            LOG_DEBUG(log, "[{func}] Invoking RealtimeMacroExecutor, full step: {step}", __func__, step_data.to_string());
        });
        
        auto step_start = step_data.start_time;
        while (step_start < step_data.end_time)
        {
            wait_for_realtime_sync(step_start);

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
