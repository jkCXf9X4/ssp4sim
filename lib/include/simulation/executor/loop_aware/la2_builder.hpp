#pragma once

#include "executor_base.hpp"
#include "shared_config.hpp"

#include <memory>
#include <vector>

namespace ssp4sim::graph
{
    /// @brief Assembles the loop-aware executor stack from raw models.
    ///
    /// Pure construction (no global Config): SCC detection, one
    /// SubstepExecutor per loop SCC (linear / shrinking `factor` sub-steps),
    /// component-DAG condensation, the outer SerialSeidel over the condensed
    /// graph, and the stack-wide read-path resolver — which is *also derived
    /// and installed here*, not inside an executor constructor. The returned
    /// executor is the outer Gauss-Seidel walk; ExecutorBuilder wraps it in a
    /// MacroExecutor.
    ///
    /// `options.parallel` requests ParallelSeidel as the outer executor; its
    /// invoke() is still stubbed, so this throws instead of failing at the
    /// first macro step. Remove the guard once ParallelSeidel::invoke lands.
    ///
    /// `options` is the `ssp4sim::La2Options` nested inside
    /// `ssp4sim::ExecutorOptions` (parsed centrally by SharedConfig) — passed
    /// straight through, never rebuilt by callers.
    std::shared_ptr<ExecutorBase> make_la2_stack(
        std::vector<std::shared_ptr<Invocable>> nodes,
        const ssp4sim::La2Options &options);
}