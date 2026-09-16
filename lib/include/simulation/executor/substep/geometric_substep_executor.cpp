#include "executor/substep/geometric_substep_executor.hpp"

#include "executor/substep/linear_substep_executor.hpp"
#include "executor_utils.hpp"

#include <cmath>
#include <cstdint>
#include <sstream>
#include <utility>

namespace ssp4sim::graph
{

    GeometricSubstepExecutor::GeometricSubstepExecutor(
        std::vector<std::shared_ptr<Invocable>> nodes,
        double factor,
        std::uint64_t threshold,
        const bool realtime)
        : ExecutorBase(std::move(nodes), "ssp4sim.execution.GeometricSubstepExecutor",
                       realtime),
          factor(factor),
          threshold(threshold)
    {
        this->name = "GeometricSubstepExecutor";

        // Out-of-range shrink factor
        if (factor <= 0.0 || factor >= 1.0)
        {
            throw std::runtime_error("GeometricSubstepExecutor, need a factor between 0.0 and 1.0");
        }
    }

    std::vector<std::pair<std::uint64_t, std::uint64_t>>
    GeometricSubstepExecutor::build_schedule(std::uint64_t start, std::uint64_t end,
                                             double factor, std::uint64_t threshold)
    {
        if (factor <= 0.0 || factor >= 1.0)
        {
            throw std::runtime_error("GeometricSubstepExecutor, need a factor between 0.0 and 1.0");
        }

        std::vector<std::pair<std::uint64_t, std::uint64_t>> out;
        if (start >= end)
        {
            return out;
        }
        const auto macro = end - start;

        const double r = factor;

        // Free-shrink: step k covers `factor` of the remaining time, so the
        // cumulative end after k steps is 1 - factor^k. Keep shrinking while
        // the remaining time stays above `threshold`; once it drops at or
        // below `threshold`, the remaining step is taken whole. With no early
        // stop (threshold == 0) the series still terminates: the rounded ends
        // eventually stop advancing (llround collisions).
        //
        // Contract: the union of the emitted sub-steps must exactly equal
        // [start, end]. Every early-stop path below can leave
        // [prev, end) uncovered, so the tail is closed after the loop: the
        // last emitted sub-step is extended to `end` (or a final (prev, end)
        // sub-step is emitted when nothing was emitted yet). Only this final
        // sub-step may be shorter than `threshold`.
        auto clamp = [&](std::size_t k) {
            auto end_at = start
                          + static_cast<std::uint64_t>(std::llround(
                              static_cast<double>(macro) * (1.0 - std::pow(r, static_cast<double>(k)))));
            return std::min(end_at, end);
        };

        auto prev = start;
        for (std::size_t k = 1; prev < end; ++k)
        {
            auto curr_end = clamp(k);
            if (curr_end <= prev)
            {
                // Rounding stopped advancing the series; fall through to the
                // tail-close below.
                break;
            }
            out.emplace_back(prev, curr_end);
            prev = curr_end;

            // The remaining time is at or below the threshold: take the rest
            // of the macro step whole.
            if (end - prev <= threshold)
            {
                break;
            }
        }

        // Close the tail so [start, end] is fully covered (see contract above).
        if (prev < end)
        {
            if (out.empty())
            {
                out.emplace_back(prev, end);
            }
            else
            {
                out.back().second = end;
            }
        }
        return out;
    }

    uint64_t GeometricSubstepExecutor::invoke(StepData step_data)
    {
        IF_LOG({
            LOG_DEBUG(log, "[{func}] {name}: {n} nodes, factor {factor}, "
                           "threshold {threshold} ns",
                      __func__, name, nodes.size(), factor, threshold);
        });

        for (auto &[sub_start, sub_end] : build_schedule(
                 step_data.start_time, step_data.end_time,
                 factor, threshold))
        {
            wait_for_realtime_sync(sub_start);
            invoke_group_parallel(nodes, StepData(sub_start, sub_end));
        }
        return step_data.end_time;
    }

    std::string GeometricSubstepExecutor::to_string() const
    {
        std::ostringstream oss;
        oss << "GeometricSubstepExecutor: " << nodes.size() << " nodes, factor "
            << factor << ", threshold " << threshold << " ns\n";
        return oss.str();
    }

}