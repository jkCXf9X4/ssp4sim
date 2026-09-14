#pragma once

#include "executor_base.hpp"
#include "invocable.hpp"

#include "executor/substep/substep_schedule.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ssp4sim::graph
{

    /**
     * @brief Sequential-DAG scheduler: resolves SCCs, sets up a sequential DAG
     *        with the strongly connected components as single nodes, and runs the
     *        DAG Gauss-Seidel-style. Every multi-node SCC (loop) is executed in
     *        parallel as a part of the sequential DAG.
     *
     * Analysis (constructor): runs GraphAnalysis to resolve SCCs, build the
     * component DAG and topologically sort it. Execution walks the components in
     * topological order: single-node components run sequentially with the full
     * macro step; multi-node loop components are relaxed over progressive
     * sub-steps.
     *
     * Example graph:
     *   A -> B -> C -> B (loop {B, C}); C -> D
     * 1. A
     * 2. B and C in parallel (loop group)
     * 3. D
     *
     * The parallel (loop group) part runs in two modes, configured via
     * `simulation.executor.la2.mode`:
     *   "linear" - pre-selected equal sub-steps, run until the full macro step
     *              is passed. The sub-step count is `simulation.executor.la2.iterations`
     *              (default: SCC node count).
     *   "factor"  - each sub-step is `simulation.executor.la2.factor` of the time
     *              left to the current macro step: a factor of 0.5 first takes
     *              macro/2, then macro/4, then macro/8, ... The sub-steps are
     *              generated until the next one would be smaller than
     *              `simulation.executor.la2.min_substep_fraction` of the macro
     *              step (default 0.001), capped by `simulation.executor.la2.max_steps`
     *              (default 64).
     *
     * Loop sub-steps advance time (models cannot be reset), so each sub-step
     * moves the loop group forward toward the macro-step end.
     */
    class La2Scheduler final : public ExecutorBase
    {
    public:
        // SCCs computed from the graph (moved from a constructor-local analysis).
        std::vector<std::vector<Invocable *>> sccs;

        // Topologically ordered indices into sccs. Execution walks this list.
        std::vector<std::size_t> execution_order;

        // For each SCC index, the number of loop sub-steps to run.
        // 1 for non-loops (single-node SCCs), >1 for multi-node loops.
        std::vector<std::size_t> loop_iterations;

        // ---- Construction ---------------------------------------------------

        La2Scheduler(std::vector<std::shared_ptr<Invocable>> nodes);

        std::string to_string() const override;

        // Walk the condensed DAG in topological order: single-node components
        // run sequentially with the full macro step; loop components run via run_loop.
        uint64_t invoke(StepData step_data) override final;

    private:
        // Cached from the constructor (the analysis is a constructor-local);
        // invoke() dispatches on this instead of re-deriving loop-ness.
        std::vector<bool> is_loop;

        // Per-SCC sub-step schedule template, resolved once in the constructor
        // so run_loop() doesn't re-read config on every macro step. For Factor
        // free-shrink the min-substep is a macro fraction, so the absolute value
        // is filled in per step.
        std::vector<substep::Config> loop_config;

        // Config read once in the constructor: for Factor free-shrink the
        // min-substep threshold is macro-relative, so the absolute value is
        // computed per macro step from this cached fraction.
        double min_substep_fraction = 0.001;

        // Execute a single-node component: each node runs once with the full
        // macro step (output written at the macro end so downstream components
        // read fresh data).
        void run_sequential(std::size_t scc_idx, const StepData &step);

        // Execute a multi-node SCC (loop): run its nodes in parallel for each
        // sub-step of the configured schedule (linear or factor mode). Loop
        // group nodes are owned by this scheduler (ExecutorBase::nodes); the
        // per-sub-step parallel invocation uses the shared invoke_group_parallel
        // kernel (executor_utils.hpp).
        void run_loop(std::size_t scc_idx, const StepData &step);
    };

}