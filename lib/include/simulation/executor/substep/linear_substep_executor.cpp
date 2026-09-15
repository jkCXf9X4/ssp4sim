#include "executor/substep/linear_substep_executor.hpp"

#include "executor_utils.hpp"

#include <algorithm>
#include <cstdint>
#include <sstream>
#include <utility>

namespace ssp4sim::graph
{

    LinearSubstepExecutor::LinearSubstepExecutor(
        std::vector<std::shared_ptr<Invocable>> nodes, std::size_t iterations)
        : ExecutorBase(std::move(nodes), "ssp4sim.execution.LinearSubstepExecutor"),
          iterations(iterations)
    {
        this->name = "LinearSubstepExecutor";
    }

    std::vector<std::pair<std::uint64_t, std::uint64_t>>
    LinearSubstepExecutor::build_schedule(std::uint64_t start, std::uint64_t end,
                                          std::size_t steps)
    {
        std::vector<std::pair<std::uint64_t, std::uint64_t>> out;
        if (start >= end)
        {
            return out;
        }

        auto n = steps ? steps : std::size_t(1);
        auto sub_dt = (end - start) / n;
        if (sub_dt == 0u)
        {
            sub_dt = 1u;
        }
        auto t = start;
        while (t < end)
        {
            auto e = std::min(t + sub_dt, end);
            out.emplace_back(t, e);
            t = e;
        }
        return out;
    }

    uint64_t LinearSubstepExecutor::invoke(StepData step_data)
    {
        IF_LOG({
            LOG_DEBUG(log, "[{func}] {name}: {n} nodes, {steps} sub-steps",
                      __func__, name, nodes.size(), iterations);
        });

        for (auto &[sub_start, sub_end] : build_schedule(
                 step_data.start_time, step_data.end_time, iterations))
        {
            invoke_group_parallel(nodes, StepData(sub_start, sub_end));
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