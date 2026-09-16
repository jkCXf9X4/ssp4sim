#pragma once

#include "executor_base.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ssp4sim::graph
{
    /// @brief Runs a group of nodes over shrinking sub-steps (each sub-step
    ///        covers `factor` of the remaining time).
    ///
    /// Each sub-step covers `factor` of the remaining time. Shrinking stops
    /// once the remaining time is at or below `threshold` (absolute ns): the
    /// remaining step is then taken whole, so [start, end] is always fully
    /// covered and only the final sub-step may be shorter than `threshold`.
    ///
    /// invoke() sweeps the whole group in parallel per sub-step
    /// (invoke_group_parallel). Sub-steps advance time, so each sub-step
    /// samples the previous sub-step's commitments (deterministic Jacobi-style
    /// relaxation).
    ///
    /// `realtime` (default false) paces every emitted sub-step to the wall
    /// clock via ExecutorBase::wait_for_realtime_sync.
    ///
    /// Resolver-neutral: this executor never installs a read-path resolver.
    /// The executor stack owner installs THE resolver for the whole stack.
    class GeometricSubstepExecutor final : public ExecutorBase
    {
    public:
        GeometricSubstepExecutor(std::vector<std::shared_ptr<Invocable>> nodes,
                                 double factor = 0.5,
                                 std::uint64_t threshold = 0,
                                 const bool realtime = false);

        // Shrinking sub-steps over [start, end): each covers `factor` of the
        // remaining time; when the remaining time drops at or below
        // `threshold` (absolute ns) the remaining step is taken whole. An
        // out-of-range factor (not in (0, 1)) throws. Contract: the union of
        // the emitted sub-steps always exactly equals [start, end].
        static std::vector<std::pair<std::uint64_t, std::uint64_t>> build_schedule(
            std::uint64_t start, std::uint64_t end,
            double factor, std::uint64_t threshold);

        std::string to_string() const override;

        uint64_t invoke(StepData step_data) override final;

    private:
        double factor = 0.5;
        // Absolute remaining-time cutoff (ns): once the remaining time drops
        // at or below `threshold`, the rest of the macro step is taken whole.
        std::uint64_t threshold = 0;
    };
}