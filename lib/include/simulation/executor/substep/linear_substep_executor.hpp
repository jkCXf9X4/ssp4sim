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
    /// invoke() divides [start, end) into `iterations` equal sub-steps and
    /// sweeps the whole group in parallel per sub-step (invoke_group_parallel).
    /// Each sub-step is total / iterations (floor), with the total % iterations
    /// remainder distributed one unit per early sub-step, so sub-steps land
    /// exactly on end. Sub-steps advance time,
    /// so each sub-step samples the previous sub-step's commitments
    /// (deterministic Jacobi-style relaxation).
    ///
    /// `realtime` (default false) paces every emitted sub-step to the wall
    /// clock via ExecutorBase::wait_for_realtime_sync.
    ///
    /// Resolver-neutral: this executor never installs a read-path resolver.
    /// The executor stack owner installs THE resolver for the whole stack.
    class LinearSubstepExecutor final : public ExecutorBase
    {
    public:
        LinearSubstepExecutor(std::vector<std::shared_ptr<Invocable>> nodes,
                              std::size_t iterations = 0,
                              const bool realtime = false);


        std::string to_string() const override;

        uint64_t invoke(StepData step_data) override final;

    private:
        std::size_t iterations = 0;
    };
}