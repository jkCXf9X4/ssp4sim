#pragma once

#include "executor_base.hpp"

#include <cstddef>
#include <memory>
#include <string>
#include <vector>

namespace ssp4sim::graph
{
    /// Static parameters for the loop-aware (la2) executor stack. Filled by
    /// ExecutorBuilder from `simulation.executor.la2.*` config; the assembly
    /// itself reads no global Config.
    struct La2Options
    {
        std::string mode = "linear";          // "linear" | "factor"
        int iterations = -1;                  // < 0 -> SCC node count
        double factor = 0.8;
        std::size_t max_steps = 64;
        double min_substep_fraction = 0.001;
        bool parallel = false;
    };

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
    std::shared_ptr<ExecutorBase> make_la2_stack(
        std::vector<std::shared_ptr<Invocable>> nodes,
        const La2Options &options);
}