#include "executor/substep/linear_substep_executor.hpp"

#include "executor_utils.hpp"

#include <algorithm>
#include <cstdint>
#include <sstream>
#include <utility>

namespace ssp4sim::graph
{

    LinearSubstepExecutor::LinearSubstepExecutor(
        std::vector<std::shared_ptr<Invocable>> nodes, std::size_t iterations,
        const bool realtime)
        : ExecutorBase(std::move(nodes), "ssp4sim.execution.LinearSubstepExecutor",
                       realtime),
          iterations(iterations)
    {
        this->name = "LinearSubstepExecutor";

        if (iterations < 1)
        {
            throw std::runtime_error("LinearSubstepExecutor need to take at least 1 iteration, currently: " + std::to_string(iterations));
        }
    }

    uint64_t LinearSubstepExecutor::invoke(StepData step_data)
    {
        IF_LOG({
            LOG_DEBUG(log, "[{func}] {name}: {n} nodes, {steps} sub-steps",
                      __func__, name, nodes.size(), iterations);
        });

        const auto total = step_data.end_time - step_data.start_time;
        const auto base_sub_dt = total / iterations;
        const auto remainder = total % iterations;

        auto start = step_data.start_time;

        for (uint64_t i = 0; i < iterations; ++i)
        {

            // distribute the remainders across the iterations
            const auto end = start + base_sub_dt + (i < remainder);

            wait_for_realtime_sync(start);
            invoke_group_parallel(nodes, StepData(start, end));

            start = end;
        }

        return step_data.end_time;
    }

    std::string LinearSubstepExecutor::to_string() const
    {
        std::ostringstream oss;
        oss << "LinearSubstepExecutor: " << nodes.size() << " nodes, "
            << iterations << " sub-steps\n";
        return oss.str();
    }

}