#pragma once

#include "executor_base.hpp"
#include "invocable.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ssp4sim::graph
{
    /// @brief Loop-aware v2 scheduler: a thin execution shell over the
    ///        pre-assembled loop-aware stack.
    ///
    /// The stack itself (SCC detection, per-loop SubstepExecutors, the
    /// condensed component DAG, the outer Gauss-Seidel executor and the
    /// stack-wide read-path resolver) is assembled by ExecutorBuilder from
    /// configuration, so this executor reads no global Config and owns no
    /// assembly logic. Construction only:
    ///   1. installs THE read-path resolver for the whole stack (a single
    ///      La2DataAccessResolver over all models), overwriting any default
    ///      resolver an inner executor installed;
    ///   2. retains the pre-assembled `outer` executor, which dequeues the
    ///      actual Gauss-Seidel walk.
    ///
    /// Loop sub-steps advance time (models cannot be reset), so each sub-step
    /// moves the loop group forward toward the macro-step end (`linear` equal
    /// sub-steps or `factor` shrinking sub-steps).
    class La2Scheduler final : public ExecutorBase
    {
    public:
        /// `nodes` are the raw models (the schedule runs over them), `outer`
        /// is the pre-assembled executor stack the builder condensed the graph
        /// into, and `resolver` is the stack-wide resolver the builder derived
        /// from the SCC partition.
        La2Scheduler(std::vector<std::shared_ptr<Invocable>> nodes,
                     std::shared_ptr<ExecutorBase> outer,
                     std::shared_ptr<DataAccessResolver> resolver);

        std::string to_string() const override;

        // Delegate the Gauss-Seidel walk to the outer executor.
        uint64_t invoke(StepData step_data) override final;

    private:
        std::shared_ptr<ExecutorBase> outer;
    };
}