#include "execution/loop_aware/loop_aware_executor.hpp"

#include "execution/executor_utils.hpp"

#include "utils/graph/tarjan.hpp"

#include <algorithm>
#include <cstdint>
#include <map>
#include <queue>
#include <set>
#include <sstream>
#include <vector>

namespace ssp4sim::graph
{

    // =========================================================================
    //  Construction
    // =========================================================================

    LoopAwareExecutor::LoopAwareExecutor(std::vector<Invocable *> nodes)
        : ExecutionBase(nodes),
          log(ssp4cpp::utils::log::make_logger("ssp4sim.execution.LoopAwareExecutor"))
    {
        this->name = "LoopAwareExecutor";
        LOG_INFO(log, "[{func}] Analysing graph for SCCs", __func__);
        analyze_graph();

        // Create JacobiParallelTBB instances for loop SCC groups.
        loop_executors.resize(sccs.size());
        for (std::size_t i = 0; i < sccs.size(); ++i)
        {
            if (loop_iterations[i] > 1)
            {
                LOG_INFO(log, "[{func}] Creating JacobiParallelTBB for loop SCC #{idx} ({size} nodes)",
                         __func__, i, sccs[i].size());
                loop_executors[i] = std::make_unique<JacobiParallelTBB>(sccs[i]);
            }
        }

        LOG_INFO(log, "[{func}] Found {n} SCCs, execution order has {m} steps",
                 __func__, sccs.size(), execution_order.size());
        for (std::size_t i = 0; i < execution_order.size(); ++i)
        {
            auto idx = execution_order[i];
            auto &comp = sccs[idx];
            LOG_INFO(log, "[{func}]   Step {step}: SCC #{idx} ({size} nodes, {iter} iters)",
                     __func__, i, idx, comp.size(), loop_iterations[idx]);
            for (auto *node : comp)
            {
                LOG_INFO(log, "[{func}]     - {name}", __func__, node->name);
            }
        }
    }

    std::string LoopAwareExecutor::to_string() const
    {
        std::ostringstream oss;
        oss << "LoopAwareExecutor:\n";
        oss << "  SCCs: " << sccs.size() << "\n";
        oss << "  Execution steps: " << execution_order.size() << "\n";
        for (std::size_t i = 0; i < execution_order.size(); ++i)
        {
            auto idx = execution_order[i];
            auto &comp = sccs[idx];
            oss << "  Step " << i << " (SCC #" << idx << ", " << comp.size()
                << " nodes, " << loop_iterations[idx] << " iters):\n";
            for (auto *node : comp)
            {
                oss << "    - " << node->name << "\n";
            }
        }
        return oss.str();
    }

    // =========================================================================
    //  Graph analysis
    // =========================================================================

    void LoopAwareExecutor::analyze_graph()
    {
        // 1. Run Tarjan's SCC on the full node set.
        auto raw_sccs = ssp4sim::utils::graph::strongly_connected_components(
            ssp4sim::utils::graph::Node::cast_to_parent_ptrs(nodes));

        // Cast back to Invocable*.
        sccs.clear();
        for (auto &raw : raw_sccs)
        {
            std::vector<Invocable *> comp;
            for (auto *n : raw)
            {
                comp.push_back(static_cast<Invocable *>(n));
            }
            sccs.push_back(std::move(comp));
        }

        // 2. Build a map from node pointer → SCC index.
        std::map<Invocable *, std::size_t> node_to_scc;
        for (std::size_t i = 0; i < sccs.size(); ++i)
        {
            for (auto *node : sccs[i])
            {
                node_to_scc[node] = i;
            }
        }

        // 3. Build the SCC DAG.
        auto dag = build_scc_dag();

        // 4. Topologically sort the SCC DAG.
        execution_order = topological_sort(dag);

        // 5. Compute loop iterations for each SCC.
        loop_iterations.resize(sccs.size(), 1);
        for (std::size_t i = 0; i < sccs.size(); ++i)
        {
            if (sccs[i].size() > 1)
            {
                // For loops, iterate loop_size times so every node sees every
                // other node's output once.
                loop_iterations[i] = sccs[i].size();
            }
        }
    }

    std::map<std::size_t, std::set<std::size_t>> LoopAwareExecutor::build_scc_dag() const
    {
        // Build node → SCC index map.
        std::map<Invocable *, std::size_t> node_to_scc;
        for (std::size_t i = 0; i < sccs.size(); ++i)
        {
            for (auto *node : sccs[i])
            {
                node_to_scc[node] = i;
            }
        }

        std::map<std::size_t, std::set<std::size_t>> dag;
        for (std::size_t i = 0; i < sccs.size(); ++i)
        {
            dag[i] = {};
            for (auto *node : sccs[i])
            {
                for (auto *child : node->children)
                {
                    auto child_scc = node_to_scc.at(static_cast<Invocable *>(child));
                    if (child_scc != i)
                    {
                        dag[i].insert(child_scc);
                    }
                }
            }
        }
        return dag;
    }

    std::vector<std::size_t> LoopAwareExecutor::topological_sort(
        std::map<std::size_t, std::set<std::size_t>> dag) const
    {
        // Compute in-degree for each SCC.
        std::map<std::size_t, std::size_t> in_degree;
        for (auto &[node, _] : dag)
        {
            in_degree[node] = 0;
        }
        for (auto &[_, targets] : dag)
        {
            for (auto &t : targets)
            {
                in_degree[t]++;
            }
        }

        // Kahn's algorithm.
        std::queue<std::size_t> q;
        for (auto &[node, deg] : in_degree)
        {
            if (deg == 0)
            {
                q.push(node);
            }
        }

        std::vector<std::size_t> order;
        while (!q.empty())
        {
            auto node = q.front();
            q.pop();
            order.push_back(node);

            for (auto &t : dag[node])
            {
                in_degree[t]--;
                if (in_degree[t] == 0)
                {
                    q.push(t);
                }
            }
        }

        if (order.size() != dag.size())
        {
            LOG_ERROR(log, "[{func}] Cycle detected in SCC DAG! "
                           "Ordered {ordered} of {total} components",
                      __func__, order.size(), dag.size());
        }

        return order;
    }

    // =========================================================================
    //  Hot path — execution
    // =========================================================================

    uint64_t LoopAwareExecutor::invoke(StepData step_data)
    {
        IF_LOG({
            LOG_DEBUG(log, "[{func}] stepdata: {stepdata}", __func__, step_data.to_string());
        });
        auto macro_dt = step_data.end_time - step_data.start_time;

        // Walk the SCC DAG in topological order.
        for (auto scc_idx : execution_order)
        {
            auto &comp = sccs[scc_idx];
            auto n_iters = loop_iterations[scc_idx];

            // Sequential (non-loop) SCC: write output at t_start so
            // subsequent models in the chain can read it immediately
            // (Gauss-Seidel principle). Parallel loop SCCs continue
            // to write at t_end.
            if (n_iters == 1)
            {
                // Non-loop SCC: execute each node sequentially.
                // These nodes have no cyclic dependencies, so a single pass suffices.
                // Write output at macro timestep end so the loop SCC
                // can interpolate between the previous and current macro
                // timestep outputs at sub-step times.
                auto s = StepData(step_data.start_time,
                                  step_data.end_time,
                                  macro_dt,
                                  step_data.start_time,
                                  step_data.end_time);

                // should always be 1?
                for (auto *node : comp)
                {
                    node->invoke(s);
                }
            }
            else
            {
                // Loop SCC: delegate to JacobiParallelTBB for parallel execution.
                // Run n_iters iterations with a reduced timestep so that every
                // node in the loop gets a chance to exchange messages.

                IF_LOG({
                    LOG_DEBUG(log, "[{func}] Loop SCC #{idx} ({size} nodes, "
                                   "{iters} iters, loop_dt={dt})",
                              __func__, scc_idx, comp.size(), n_iters, loop_dt);
                });

                auto *loop_exec = loop_executors[scc_idx].get();
    
                auto sub_dt = macro_dt / n_iters;
                auto sub_start = step_data.start_time;

                while (sub_start < step_data.end_time)
                {
                    auto sub_end = std::min(sub_start + sub_dt, step_data.end_time);
                    auto step = sub_end - sub_start;
                    auto s = StepData(sub_start, sub_end, step, sub_start, sub_end);
                    
                    loop_exec->invoke(s);
                    sub_start = sub_end;
                }
            }
        }

        return step_data.end_time;
    }

}
