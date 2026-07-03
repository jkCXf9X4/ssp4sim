#pragma once

#include "ssp4sim_definitions.hpp"

#include "execution/executor.hpp"
#include "execution/invocable.hpp"

#include "execution/jacobi/jacobi_parallel_tbb.hpp"

#include "utils/graph/tarjan.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace ssp4sim::graph
{

    /**
     * @brief A strongly-connected-component-aware executor.
     *
     * Analysis phase (constructor):
     * 1. Run Tarjan's SCC on the full node graph.
     * 2. Build a DAG of SCCs (component → component edges).
     * 3. Topologically sort the component DAG.
     * 4. For each multi-node SCC (loop), create a JacobiParallelTBB instance
     *    to handle parallel execution within the loop group.
     *
     * Execution phase (invoke):
     * 1. Walk the sorted components in order.
     * 2. For single-node SCCs (non-loop): execute directly with the full timestep.
     * 3. For multi-node SCCs (loop): delegate to the JacobiParallelTBB instance
     *    for each sub-step, running `loop_size` iterations with a sub-timestep
     *    of `macro_timestep / loop_size`, giving every node in the loop a chance
     *    to see every other node's output.
     */
    class LoopAwareExecutor final : public ExecutionBase
    {
    public:
        ssp4cpp::utils::log::Logger *log = nullptr;

        // ---- Analysis results ----
        // SCCs computed from the graph, each is a vector of nodes.
        std::vector<std::vector<Invocable *>> sccs;

        // Topologically ordered indices into sccs.
        // Execution walks this list sequentially.
        std::vector<std::size_t> execution_order;

        // For each SCC index, the number of loop iterations to run.
        // 1 for non-loops (single-node SCCs), >1 for multi-node loops.
        std::vector<std::size_t> loop_iterations;

        // For each SCC index that is a loop, a JacobiParallelTBB executor
        // that handles parallel execution of the loop group nodes.
        // Non-loop SCCs have nullptr here.
        std::vector<std::unique_ptr<JacobiParallelTBB>> loop_executors;

        // ---- Construction ---------------------------------------------------

        LoopAwareExecutor(std::vector<Invocable *> nodes);

        std::string to_string() const override;

        // ---- Hot path -------------------------------------------------------

        uint64_t invoke(StepData step_data) override final;

    private:
        // Build the SCC DAG and compute topological order.
        void analyze_graph();

        // Compute the set of target SCC indices for each source SCC.
        std::map<std::size_t, std::set<std::size_t>> build_scc_dag() const;

        // Topological sort of the SCC DAG (Kahn's algorithm).
        std::vector<std::size_t> topological_sort(
            std::map<std::size_t, std::set<std::size_t>> dag) const;
    };

}
