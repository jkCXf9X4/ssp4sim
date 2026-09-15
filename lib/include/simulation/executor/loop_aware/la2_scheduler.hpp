#pragma once

#include "executor_base.hpp"
#include "invocable.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ssp4sim::graph
{
    /**
     * @brief Loop-aware v2 scheduler: Gauss-Jacobi sub-step relaxation nested
     *        inside a generic Gauss-Seidel (Seidel) outer executor.
     *
     * Assembler / orchestrator: this scheduler owns nothing timing-related
     * itself. Construction:
     *   1. utils::graph::Graph -> SCCs, per-SCC loop classification, component DAG.
     *   2. One SubstepExecutor per loop SCC; acyclic single-node SCCs stay bare
     *      models (they run once per macro step, no sub-stepping).
     *   3. Condenses the graph: each loop SCC is replaced in the node adjacency
     *      by its executor node (utils/graph/rewire.hpp), so the outer Seidel
     *      traverses the component DAG unmodified.
     *   4. Outer executor: SerialSeidel (default) over the condensed component
     *      graph; `simulation.executor.la2.parallel` selects ParallelSeidel.
     *   5. Installs THE read-path resolver for the whole stack (a single
     *      La2DataAccessResolver over all models) as the last construction
     *      step, overwriting any default resolver an inner executor installed.
     *
     * Loop sub-steps advance time (models cannot be reset), so each sub-step
     * moves the loop group forward toward the macro-step end (`linear` equal
     * sub-steps or `factor` shrinking sub-steps).
     */
    class La2Scheduler final : public ExecutorBase
    {
    public:
        La2Scheduler(std::vector<std::shared_ptr<Invocable>> nodes);

        std::string to_string() const override;

        // Delegate the Gauss-Seidel walk to the outer executor.
        uint64_t invoke(StepData step_data) override final;

    private:
        std::shared_ptr<ExecutorBase> outer;
    };
}