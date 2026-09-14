#include "substep_schedule.hpp"

#include <algorithm>
#include <cmath>

namespace ssp4sim::graph::substep
{

    std::vector<std::pair<uint64_t, uint64_t>> build_substep_schedule(
        uint64_t start, uint64_t end, const Config &config)
    {
        std::vector<std::pair<uint64_t, uint64_t>> out;
        if (start >= end)
        {
            return out;
        }
        const auto macro = end - start;

        // Default path: equal pre-selected sub-steps (Linear mode, or an
        // invalid shrink factor treated as Linear).
        if (config.mode == Mode::Linear ||
            config.factor <= 0.0 ||
            config.factor >= 1.0)
        {
            auto n = config.steps ? config.steps : std::size_t(1);
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

        const double r = config.factor;

        // Scaled shrinking sub-steps landing exactly on the macro end (fixed count).
        // ends[steps] is always exactly `end` (frac == 1.0), so the last emitted
        // sub-step reaches the macro end; zero-length sub-steps from llround
        // collisions are dropped but the remaining sub-steps stay contiguous,
        // keeping the union exactly equal to [start, end].
        if (config.steps > 0)
        {
            const double denom = 1.0 - std::pow(r, static_cast<double>(config.steps));
            std::vector<uint64_t> ends;
            ends.reserve(config.steps + 1);
            for (std::size_t k = 0; k <= config.steps; ++k)
            {
                double frac = (k == config.steps)
                    ? 1.0
                    : (1.0 - std::pow(r, static_cast<double>(k))) / denom;
                ends.push_back(
                    start
                    + static_cast<uint64_t>(std::llround(
                        static_cast<double>(macro) * frac)));
            }
            for (std::size_t k = 0; k < config.steps; ++k)
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
                          + static_cast<uint64_t>(std::llround(
                              static_cast<double>(macro) * (1.0 - std::pow(r, static_cast<double>(k)))));
            return std::min(end_at, end);
        };

        auto prev = start;
        for (std::size_t k = 1; k <= config.max_steps && prev < end; ++k)
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
            if (clamp(k + 1) - curr_end < config.min_substep)
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

}