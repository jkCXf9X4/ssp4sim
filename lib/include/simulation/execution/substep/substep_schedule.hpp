#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

namespace ssp4sim::graph::substep
{

    enum class Mode
    {
        Linear, // Pre-selected equal sub-steps covering (clamped to) the macro end.
        Factor    // Shrinking sub-steps: each sub-step is `factor` of the time left.
                  // Optionally scaled to land exactly on the macro end (fixed step count).
    };

    struct Config
    {
        // Scheduling policy.
        Mode mode = Mode::Linear;

        // Linear: number of equal sub-steps (0 -> a single full step).
        // Factor: if > 0, exactly `steps` shrinking sub-steps that land on the
        // macro end (geometric scaling). If 0, free-shrink: keep taking `factor`
        // of the remaining time until the next sub-step would be shorter than
        // `min_substep` (or `max_steps` is reached).
        std::size_t steps = 0;

        // Shrink rate in (0, 1) for Mode::Factor.
        double factor = 0.5;

        // Free-shrink stop: drop a candidate sub-step shorter than this. Only
        // the final sub-step covering the tail may be shorter than this.
        uint64_t min_substep = 0;

        // Free-shrink hard cap on the number of sub-steps.
        std::size_t max_steps = 256;
    };

    // Build the sub-step schedule for the macro step [start, end).
    // Contract: the union of the emitted sub-steps always exactly equals
    // [start, end] in every mode, so the caller can iterate the schedule and
    // simulate the whole macro interval.
    // Linear: equal pre-selected sub-steps, clamped so the schedule fully covers
    // the macro step. Factor with steps > 0: exactly `steps` shrinking sub-steps
    // ending at `end`. Factor with steps == 0: shrink by `factor` until the next
    // candidate sub-step is smaller than `min_substep` (or max_steps is reached),
    // then close the tail so the last sub-step reaches `end`. Only the final
    // sub-step may be shorter than `min_substep`.
    std::vector<std::pair<uint64_t, uint64_t>> build_substep_schedule(
        uint64_t start, uint64_t end, const Config &config);

}