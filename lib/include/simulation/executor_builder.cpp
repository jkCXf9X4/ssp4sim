#include "config.hpp"
#include "executor_builder.hpp"

#include "executor/custom/custom_executors.hpp"

#include "executor/jacobi/jacobi_parallel_fut.hpp"
#include "executor/jacobi/jacobi_parallel_spin.hpp"
#include "executor/jacobi/jacobi_parallel_tbb.hpp"
#include "executor/jacobi/jacobi_serial.hpp"

#include "executor/seidel/seidel_serial.hpp"
#include "executor/seidel/seidel_parallel.hpp"
#include "executor/loop_aware/la2_scheduler.hpp"
#include "executor/macro/macro_executor.hpp"
#include "executor/macro/realtime_macro_executor.hpp"

#include "executor/substep/geometric_substep_executor.hpp"
#include "executor/substep/linear_substep_executor.hpp"

#include "resolver/la2_data_access_resolver.hpp"

#include "utils/graph/graph.hpp"
#include "utils/graph/rewire.hpp"
#include "utils/time/time.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace ssp4sim::graph
{

    std::string ExecutorBuilder::to_string() const
    {
        return "ExecutorBuilder:\n{}\n";
    }

    std::shared_ptr<ExecutorBase> ExecutorBuilder::build(std::vector<std::shared_ptr<Invocable>> nodes)
    {
        // Each concrete executor receives its static parameters via its
        // constructor (see the per-method branches below); this builder is the
        // single place that reads the global Config for executor selection and
        // tuning, and the only place that assembles nested executor stacks.

        std::shared_ptr<ExecutorBase> specialized_executor;

        auto executor_method = utils::Config::getOr("simulation.executor.method", std::string("jacobi"));
        int workers = utils::Config::getOr("simulation.executor.thread_pool_workers", 5);

        if (executor_method == "jacobi")
        {
            if (utils::Config::getOr("simulation.executor.jacobi.parallel", false))
            {
                int parallel_method = utils::Config::getOr("simulation.executor.jacobi.method", 1);

                if (parallel_method == 1)
                {
                    LOG_INFO(log, "[{func}] Executor: JacobiParallelTBB", __func__);
                    specialized_executor = std::make_shared<JacobiParallelTBB>(nodes);
                }
                else if (parallel_method == 2)
                {
                    LOG_INFO(log, "[{func}] Executor: JacobiParallelSpin", __func__);
                    specialized_executor = std::make_shared<JacobiParallelSpin>(nodes, workers);
                }
                else if (parallel_method == 3)
                {
                    LOG_INFO(log, "[{func}] Executor: JacobiParallelFutures", __func__);
                    specialized_executor = std::make_shared<JacobiParallelFutures>(nodes, workers);
                }
                else
                {
                    throw std::runtime_error("Unknown parallelization method");
                }
            }
            else
            {
                LOG_INFO(log, "[{func}] Executor: JacobiSerial", __func__);
                specialized_executor = std::make_shared<JacobiSerial>(nodes);
            }
        }
        else if (executor_method == "seidel")
        {
            if (utils::Config::getOr("simulation.executor.seidel.parallel", false))
            {
                LOG_INFO(log, "[{func}] Executor: ParallelSeidel", __func__);
                specialized_executor = std::make_shared<ParallelSeidel>(nodes);
            }
            else
            {
                LOG_INFO(log, "[{func}] Executor: SerialSeidel", __func__);
                specialized_executor = std::make_shared<SerialSeidel>(nodes);
            }
        }
        else if (executor_method == "custom_delay")
        {
            LOG_INFO(log, "[{func}] Executor: DelayExecutor", __func__);
            specialized_executor = std::make_shared<DelayExecutor>(nodes);
        }
        else if (executor_method == "custom_delay_partial")
        {
            LOG_INFO(log, "[{func}] Executor: DelayExecutorPartial", __func__);
            specialized_executor = std::make_shared<DelayExecutorPartial>(nodes);
        }
        else if (executor_method == "la2" || executor_method == "loop_aware")
        {
            // "loop_aware" is the legacy name (see docs/configuration.md and
            // resources/loop_aware_nested*.json); "la2" is the refactored name.
            // Both dispatch to the same scheduler.
            LOG_INFO(log, "[{func}] Executor: La2Scheduler", __func__);
            specialized_executor = build_la2(std::move(nodes));
        }
        else if (executor_method == "parallel_seidel" || executor_method == "parallel-seidel")
        {
            // ParallelSeidel is NOT implemented: its invoke() throws
            // "This is not implemented" at runtime. Fail at dispatch time with
            // a clear, actionable message instead of silently mis-dispatching
            // or falling through to the generic "Unknown executor method".
            throw std::runtime_error(
                "Executor method '" + executor_method +
                "' requests ParallelSeidel, which is NOT implemented. "
                "Use 'seidel' (serial) or 'jacobi' instead.");
        }

        if (!specialized_executor)
        {
            throw std::runtime_error("Unknown executor method: '" + executor_method + "'");
        }

        const auto macro_step =
            utils::time::s_to_ns(utils::Config::getDouble("simulation.timestep"));

        if (utils::Config::getOr("simulation.realtime", false))
        {
            return std::make_shared<graph::RealtimeMacroExecutor>(
                std::vector<std::shared_ptr<Invocable>>{specialized_executor},
                macro_step);
        }
        else
        {
            return std::make_shared<graph::MacroExecutor>(
                std::vector<std::shared_ptr<Invocable>>{specialized_executor},
                macro_step);
        }
    }

    std::shared_ptr<ExecutorBase> ExecutorBuilder::build_la2(
        std::vector<std::shared_ptr<Invocable>> nodes)
    {
        // 1. SCC detection and component DAG. The analysis owns no node memory
        //    and dies at the end of this function; the assembled stack keeps
        //    the representatives + the outer executor below.
        std::vector<Invocable *> raw;
        raw.reserve(nodes.size());
        for (auto &node : nodes)
        {
            raw.push_back(node.get());
        }
        ssp4sim::utils::graph::Graph analysis(
            ssp4sim::utils::graph::Node::cast_to_parent_ptrs(raw));
        analysis.analyze();
        const auto &sccs = analysis.sccs();
        const auto &is_loop = analysis.is_loop();

        // Raw-pointer -> owned-node lookup, so members can be handed to their
        // component representative without changing ownership.
        std::map<Invocable *, std::shared_ptr<Invocable>> owned;
        for (auto &node : nodes)
        {
            owned[node.get()] = node;
        }

        // 2. One representative per SCC: loop SCCs are wrapped in a
        //    SubstepExecutor; acyclic single-node SCCs stay the model itself
        //    (they must NOT be sub-stepped, only loops relax over sub-steps).
        const auto mode = utils::Config::getOr(
            "simulation.executor.la2.mode", std::string("linear"));
        const auto configured_iters = utils::Config::getOr(
            "simulation.executor.la2.iterations", -1);
        const auto geometric = (mode == "factor" || mode == "geometric");

        std::vector<ssp4sim::utils::graph::Node *> representatives;
        representatives.reserve(sccs.size());
        std::vector<std::shared_ptr<Invocable>> component_children;
        component_children.reserve(sccs.size());

        for (std::size_t i = 0; i < sccs.size(); ++i)
        {
            if (!is_loop[i])
            {
                representatives.push_back(sccs[i][0]);
                component_children.push_back(
                    owned[static_cast<Invocable *>(sccs[i][0])]);
                continue;
            }

            std::vector<std::shared_ptr<Invocable>> members;
            members.reserve(sccs[i].size());
            for (auto *member : sccs[i])
            {
                members.push_back(owned[static_cast<Invocable *>(member)]);
            }

            // A loop group is any SCC that is_loop(): a multi-node SCC, or a
            // single node with a self-edge (feedback self-reference). Iteration
            // count defaults to the SCC size.
            const auto iterations = configured_iters > 0
                ? static_cast<std::size_t>(configured_iters)
                : sccs[i].size();

            std::shared_ptr<Invocable> repr;
            if (geometric)
            {
                repr = std::make_shared<GeometricSubstepExecutor>(
                    std::move(members),
                    utils::Config::getOr("simulation.executor.la2.factor", 0.8),
                    static_cast<std::size_t>(utils::Config::getOr(
                        "simulation.executor.la2.max_steps", 64)),
                    utils::Config::getOr("simulation.executor.la2.min_substep_fraction", 0.001),
                    iterations);
            }
            else
            {
                repr = std::make_shared<LinearSubstepExecutor>(
                    std::move(members), iterations);
            }

            representatives.push_back(repr.get());
            component_children.push_back(std::move(repr));
        }

        // 3. Condense the graph: loop SCCs are replaced in the adjacency by
        //    their executor node. SerialSeidel then runs the component DAG
        //    exactly as it runs any flat graph.
        ssp4sim::utils::graph::condense_component_dag(
            analysis.component_dag(), representatives);

        // 4. Outer Gauss-Seidel executor. ParallelSeidel is selected by
        //    `simulation.executor.la2.parallel`; its invoke() is still stubbed,
        //    so fail loudly here instead of at the first macro step. Remove this
        //    guard when ParallelSeidel::invoke is implemented.
        if (utils::Config::getOr("simulation.executor.la2.parallel", false))
        {
            throw std::runtime_error(
                "simulation.executor.la2.parallel requests ParallelSeidel, "
                "which is NOT implemented. Remove the guard in "
                "executor_builder.cpp once ParallelSeidel::invoke lands.");
        }
        auto outer = std::make_shared<SerialSeidel>(std::move(component_children));

        // 5. The scheduler owns the stack's read policy: one flat
        //    La2DataAccessResolver over all models, installed last so it
        //    overwrites any default resolver an inner executor set on direct
        //    FmuModel children (intra-SCC edges sample at sub-step start,
        //    cross-SCC edges read the latest committed value).
        std::vector<std::size_t> scc_of;
        for (std::size_t si = 0; si < sccs.size(); ++si)
        {
            for (auto *node : sccs[si])
            {
                const auto idx = static_cast<std::size_t>(node->id);
                if (scc_of.size() <= idx)
                {
                    scc_of.resize(idx + 1, ssp4sim::scheduling::DataAccessResolver::no_producer);
                }
                scc_of[idx] = si;
            }
        }
        auto resolver = std::make_shared<ssp4sim::scheduling::La2DataAccessResolver>(
            raw, std::move(scc_of));

        LOG_INFO(log, "[{func}] La2Scheduler: {n} SCCs, outer {outer}",
                 __func__, sccs.size(), outer->name);

        return std::make_shared<La2Scheduler>(
            std::move(nodes), std::move(outer), std::move(resolver));
    }

}
