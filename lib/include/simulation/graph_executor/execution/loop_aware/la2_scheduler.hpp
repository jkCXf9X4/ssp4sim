#pragma once

#include "executor.hpp"
#include "invocable.hpp"

#include "graph_analysis/graph_analysis.hpp"
#include "execution/jacobi/jacobi_parallel_tbb.hpp"

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
     * component DAG, topologically sort it, and produce the condensed SccGroup
     * graph. Execution walks the components in topological order: single-node
     * components run sequentially with the full macro step; multi-node loop
     * components are relaxed over progressive sub-steps.
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
    class La2Scheduler final : public ExecutionBase
    {
    public:
        ssp4cpp::utils::log::Logger *log = nullptr;

        // Reusable graph analysis (SCC detection, component DAG, topological
        // sort, condensed SccGroup graph).
        GraphAnalysis graph;

        // SCCs computed from the graph (alias into graph.sccs).
        const std::vector<std::vector<Invocable *>> &sccs;

        // Topologically ordered indices into sccs. Execution walks this list.
        const std::vector<std::size_t> &execution_order;

        // For each SCC index that is a loop, a JacobiParallelTBB executor
        // that handles parallel execution of the loop group nodes.
        std::vector<std::unique_ptr<JacobiParallelTBB>> loop_executors;

        // For each SCC index, the number of loop iterations to run.
        // 1 for non-loops (single-node SCCs), >1 for multi-node loops.
        // Only used by the "linear" mode.
        std::vector<std::size_t> loop_iterations;

        // ---- Construction ---------------------------------------------------

        La2Scheduler(std::vector<Invocable *> nodes);

        std::string to_string() const override;

        // Walk the condensed DAG in topological order: single-node components
        // run sequentially with the full macro step; loop components run via run_loop.
        uint64_t invoke(StepData step_data) override final;

    private:
        // Execute a single-node component: each node runs once with the full
        // macro step (output written at the macro end so downstream components
        // read fresh data).
        void run_sequential(std::size_t scc_idx, const StepData &step);

        // Execute a multi-node SCC (loop): run its nodes in parallel for each
        // sub-step of the configured schedule (linear or factor mode).
        void run_loop(std::size_t scc_idx, const StepData &step);
    };

}