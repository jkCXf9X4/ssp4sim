#include "executor/substep/geometric_substep_executor.hpp"

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
        std::size_t max_steps,
        double min_substep_fraction,
        std::size_t steps)
        : ExecutorBase(std::move(nodes), "ssp4sim.execution.GeometricSubstepExecutor"),
          factor(factor),
          max_steps(max_steps),
          min_substep_fraction(min_substep_fraction),
          steps(steps)
    {
        this->name = "GeometricSubstepExecutor";
    }

    std::vector<std::pair<std::uint64_t, std::uint64_t>>
    GeometricSubstepExecutor::build_schedule(std::uint64_t start, std::uint64_t end,
                                             double factor, std::size_t max_steps,
                                             std::uint64_t min_substep,
                                             std::size_t steps)
    {
        std::vector<std::pair<std::uint64_t, std::uint64_t>> out;
        if (start >= end)
        {
            return out;
        }
        const auto macro = end - start;

        // Out-of-range shrink factor: fall back to equal pre-selected
        // sub-steps (legacy contract treated an invalid factor as Linear).
        if (factor <= 0.0 || factor >= 1.0)
        {
            auto n = steps ? steps : std::size_t(1);
            auto sub_dt = macro / n;
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

        const double r = factor;

        // Scaled shrinking sub-steps landing exactly on the macro end (fixed
        // count). ends[steps] is always exactly `end` (frac == 1.0), so the
        // last emitted sub-step reaches the macro end; zero-length sub-steps
        // from llround collisions are dropped but the remaining sub-steps stay
        // contiguous, keeping the union exactly equal to [start, end].
        if (steps > 0)
        {
            const double denom = 1.0 - std::pow(r, static_cast<double>(steps));
            std::vector<std::uint64_t> ends;
            ends.reserve(steps + 1);
            for (std::size_t k = 0; k <= steps; ++k)
            {
                double frac = (k == steps)
                    ? 1.0
                    : (1.0 - std::pow(r, static_cast<double>(k))) / denom;
                ends.push_back(
                    start
                    + static_cast<std::uint64_t>(std::llround(
                        static_cast<double>(macro) * frac)));
            }
            for (std::size_t k = 0; k < steps; ++k)
            {
                if (ends[k + 1] > ends[k])
                {
                    out.emplace_back(ends[k], ends[k + 1]);
                }
            }
            return out;
        }

        // Free-shrink: step k covers `factor` of the remaining time, so the
        // cumulative end after k steps is 1 - factor^k. Stop when the next
        // candidate sub-step would be shorter than min_substep, or when the
        // rounded ends stop advancing (rounding terminates the series), or at
        // max_steps (guards endless decay when min_substep is 0).
        //
        // Contract: the union of the emitted sub-steps must exactly equal
        // [start, end] in every mode. Every early-stop path below can leave
        // [prev, end) uncovered, so the tail is closed after the loop: the
        // last emitted sub-step is extended to `end` (or a final (prev, end)
        // sub-step is emitted when nothing was emitted yet). Only this final
        // sub-step may be shorter than min_substep.
        auto clamp = [&](std::size_t k) {
            auto end_at = start
                          + static_cast<std::uint64_t>(std::llround(
                              static_cast<double>(macro) * (1.0 - std::pow(r, static_cast<double>(k)))));
            return std::min(end_at, end);
        };

        auto prev = start;
        for (std::size_t k = 1; k <= max_steps && prev < end; ++k)
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

            // Stop when the next candidate sub-step would be too small.
            if (clamp(k + 1) - curr_end < min_substep)
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
        const auto macro_dt = step_data.end_time - step_data.start_time;
        const auto min_substep = static_cast<std::uint64_t>(std::llround(
            static_cast<double>(macro_dt) * min_substep_fraction));

        IF_LOG({
            LOG_DEBUG(log, "[{func}] {name}: {n} nodes, factor {factor}, "
                           "{steps} sub-steps",
                      __func__, name, nodes.size(), factor, steps);
        });

        for (auto &[sub_start, sub_end] : build_schedule(
                 step_data.start_time, step_data.end_time,
                 factor, max_steps, min_substep, steps))
        {
            invoke_group_parallel(nodes, StepData(sub_start, sub_end));
        }
        return step_data.end_time;
    }

    std::string GeometricSubstepExecutor::to_string() const
    {
        std::ostringstream oss;
        oss << "GeometricSubstepExecutor: " << nodes.size() << " nodes, factor "
            << factor << ", " << steps << " sub-steps\n";
        return oss.str();
    }

}