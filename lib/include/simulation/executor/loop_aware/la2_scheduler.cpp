#include "executor/loop_aware/la2_scheduler.hpp"

#include "config.hpp"

#include "resolver/la2_data_access_resolver.hpp"

#include "executor/seidel/seidel_parallel.hpp"
#include "executor/seidel/seidel_serial.hpp"
#include "executor/substep/geometric_substep_executor.hpp"
#include "executor/substep/linear_substep_executor.hpp"

#include "utils/graph/graph.hpp"
#include "utils/graph/rewire.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace ssp4sim::graph
{

    // =========================================================================
    //  Construction
    // =========================================================================

    La2Scheduler::La2Scheduler(std::vector<std::shared_ptr<Invocable>> nodes)
        : ExecutorBase(std::move(nodes), "ssp4sim.execution.La2Scheduler")
    {
        this->name = "La2Scheduler";

        // 1. SCC detection and component DAG. The analysis owns no node memory
        //    and dies at the end of the constructor; the scheduler keeps the
        //    representatives + outer executor below.
        auto raw = raw_nodes();
        ssp4sim::utils::graph::Graph analysis(
            ssp4sim::utils::graph::Node::cast_to_parent_ptrs(raw));
        analysis.analyze();
        const auto &sccs = analysis.sccs();
        const auto &is_loop = analysis.is_loop();

        // Raw-pointer -> owned-node lookup, so members can be handed to their
        // component representative without changing ownership.
        std::map<Invocable *, std::shared_ptr<Invocable>> owned;
        for (auto &node : this->nodes)
        {
            owned[node.get()] = node;
        }

        // 2. One representative per SCC: loop SCCs are wrapped in a
        //    SubstepExecutor; acyclic single-node SCCs stay the model itself
        //    (they must NOT be sub-stepped, only loops relax over sub-steps).
        std::vector<ssp4sim::utils::graph::Node *> representatives;
        representatives.reserve(sccs.size());
        std::vector<std::shared_ptr<Invocable>> component_children;
        component_children.reserve(sccs.size());

        const auto mode = utils::Config::getOr(
            "simulation.executor.la2.mode", std::string("linear"));
        const auto configured_iters = utils::Config::getOr(
            "simulation.executor.la2.iterations", -1);
        const auto geometric = (mode == "factor" || mode == "geometric");

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
                "la2_scheduler.cpp once ParallelSeidel::invoke lands.");
        }
        outer = std::make_shared<SerialSeidel>(std::move(component_children));

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
        set_resolver(std::make_shared<ssp4sim::scheduling::La2DataAccessResolver>(
            raw, std::move(scc_of)));

        LOG_INFO(log, "[{func}] La2Scheduler: {n} SCCs, outer {outer}",
                 __func__, sccs.size(), outer->name);
    }

    std::string La2Scheduler::to_string() const
    {
        std::ostringstream oss;
        oss << "La2Scheduler (outer " << outer->name << ", "
            << outer->nodes.size() << " components):\n";
        return oss.str();
    }

    // =========================================================================
    //  Execution - Gauss-Seidel walk over the condensed component graph
    // =========================================================================

    uint64_t La2Scheduler::invoke(StepData step_data)
    {
        return outer->invoke(step_data);
    }

}