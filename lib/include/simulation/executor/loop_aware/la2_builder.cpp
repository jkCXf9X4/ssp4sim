#include "executor/loop_aware/la2_builder.hpp"

#include "executor/seidel/seidel_serial.hpp"
#include "executor/substep/geometric_substep_executor.hpp"
#include "executor/substep/linear_substep_executor.hpp"

#include "resolver/la2_data_access_resolver.hpp"

#include "pre/3_simulation_graph/elements/model_fmu.hpp"

#include "utils/graph/graph.hpp"
#include "utils/graph/rewire.hpp"

#include "ssp4cpp/utils/log.hpp"

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

    std::shared_ptr<ExecutorBase> make_la2_stack(
        std::vector<std::shared_ptr<Invocable>> nodes,
        const ssp4sim::La2Options &options)
    {
        auto log = ssp4cpp::utils::log::make_logger("ssp4sim.execution.La2Stack");

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
        const auto geometric = (options.mode == "factor" || options.mode == "geometric");

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
            const auto iterations = options.iterations > 0
                ? static_cast<std::size_t>(options.iterations)
                : sccs[i].size();

            std::shared_ptr<Invocable> repr;
            if (geometric)
            {
                repr = std::make_shared<GeometricSubstepExecutor>(
                    std::move(members), options.factor, options.max_steps,
                    options.min_substep_fraction, iterations);
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

        // 4. Outer Gauss-Seidel executor. ParallelSeidel is requested via
        //    `options.parallel`; its invoke() is still stubbed, so fail loudly
        //    here instead of at the first macro step.
        if (options.parallel)
        {
            throw std::runtime_error(
                "simulation.executor.la2.parallel requests ParallelSeidel, "
                "which is NOT implemented. Remove the guard in make_la2_stack "
                "once ParallelSeidel::invoke lands.");
        }
        auto outer = std::make_shared<SerialSeidel>(std::move(component_children));

        // 5. The stack's read policy: one flat La2DataAccessResolver over all
        //    models (intra-SCC edges sample at sub-step start, cross-SCC edges
        //    read the latest committed value). Derived from the SCC partition
        //    and installed here, on the assembly side, so no executor carries
        //    resolver ownership. Installed explicitly over every model so it
        //    overwrites the default resolver an inner executor set.
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
        for (auto &node : nodes)
        {
            if (auto *fmu = dynamic_cast<FmuModel *>(node.get()))
            {
                fmu->access_resolver = resolver;
            }
        }

        LOG_INFO(log, "[{func}] La2 stack: {n} SCCs, outer {outer}",
                 __func__, sccs.size(), outer->name);

        return outer;
    }

}