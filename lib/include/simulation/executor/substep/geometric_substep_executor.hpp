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
    /// Each sub-step covers `factor` of the remaining time. With `steps` > 0
    /// the shrinking sub-steps are scaled to land exactly on the macro end;
    /// with `steps` == 0 (free-shrink) the executor keeps shrinking until the
    /// next candidate sub-step would be shorter than `min_substep` (resolved by
    /// the executor from `min_substep_fraction` and the macro duration) or
    /// `max_steps` is reached, then closes the tail so [start, end] is fully
    /// covered. Only the final sub-step may be shorter than `min_substep`.
    ///
    /// invoke() sweeps the whole group in parallel per sub-step
    /// (invoke_group_parallel). Sub-steps advance time, so each sub-step
    /// samples the previous sub-step's commitments (deterministic Jacobi-style
    /// relaxation).
    ///
    /// Resolver-neutral: this executor never installs a read-path resolver.
    /// The executor stack owner installs THE resolver for the whole stack.
    class GeometricSubstepExecutor final : public ExecutorBase
    {
    public:
        GeometricSubstepExecutor(std::vector<std::shared_ptr<Invocable>> nodes,
                                 double factor = 0.5,
                                 std::size_t max_steps = 256,
                                 double min_substep_fraction = 0.001,
                                 std::size_t steps = 0);

        // Shrinking sub-steps over [start, end). `min_substep` is absolute;
        // `steps` == 0 selects free-shrink. An out-of-range factor (not in
        // (0, 1)) falls back to equal sub-steps. Contract: the union of the
        // emitted sub-steps always exactly equals [start, end].
        static std::vector<std::pair<std::uint64_t, std::uint64_t>> build_schedule(
            std::uint64_t start, std::uint64_t end,
            double factor, std::size_t max_steps,
            std::uint64_t min_substep, std::size_t steps);

        std::string to_string() const override;

        uint64_t invoke(StepData step_data) override final;

    private:
        double factor = 0.5;
        std::size_t max_steps = 256;
        // Macro-relative minimum sub-step (default 0.001 of the macro step);
        // resolved to the absolute `min_substep` per invoke().
        double min_substep_fraction = 0.001;
        std::size_t steps = 0;
    };
}