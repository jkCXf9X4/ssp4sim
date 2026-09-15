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
    /// @brief Runs a group of nodes over equal pre-selected sub-steps covering
    ///        the macro step.
    ///
    /// invoke() divides [start, end) into `iterations` equal sub-steps
    /// (0 -> a single full step) and sweeps the whole group in parallel per
    /// sub-step (invoke_group_parallel). Sub-steps advance time, so each
    /// sub-step samples the previous sub-step's commitments (deterministic
    /// Jacobi-style relaxation).
    ///
    /// Resolver-neutral: this executor never installs a read-path resolver.
    /// The executor stack owner installs THE resolver for the whole stack.
    class LinearSubstepExecutor final : public ExecutorBase
    {
    public:
        LinearSubstepExecutor(std::vector<std::shared_ptr<Invocable>> nodes,
                              std::size_t iterations = 0);

        // Equal sub-steps over [start, end); `steps` == 0 means a single full
        // step. Contract: the union of the emitted sub-steps always exactly
        // equals [start, end]. The final sub-step is clamped at `end` when the
        // macro duration is not divisible by `steps`.
        static std::vector<std::pair<std::uint64_t, std::uint64_t>> build_schedule(
            std::uint64_t start, std::uint64_t end, std::size_t steps);

        std::string to_string() const override;

        uint64_t invoke(StepData step_data) override final;

    private:
        std::size_t iterations = 0;
    };
}