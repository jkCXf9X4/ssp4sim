#include "executor/loop_aware/la2_scheduler.hpp"

#include "executor_utils.hpp"

#include "config.hpp"

#include "graph_analysis/graph_analysis.hpp"

#include "resolver/data_access_resolver.hpp"
#include "resolver/la2_data_access_resolver.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace ssp4sim::graph
{

    // =========================================================================
    //  Construction
    // =========================================================================

    La2Scheduler::La2Scheduler(std::vector<std::shared_ptr<Invocable>> nodes)
        : ExecutorBase(nodes, "ssp4sim.execution.La2Scheduler")
    {
        this->name = "La2Scheduler";

        // SCC detection and topological order are needed only to build the
        // schedule; the analysis owns no node memory and dies at the end of the
        // constructor. The scheduler keeps its own copies (`sccs`,
        // `execution_order`, `is_loop`, `loop_config`) so it stays movable
        // without aliasing.
        GraphAnalysis analysis(raw_nodes());
        analysis.analyze();

        sccs = std::move(analysis.sccs);
        execution_order = std::move(analysis.execution_order);
        is_loop = std::move(analysis.is_loop);
        loop_iterations.resize(sccs.size(), 1);
        loop_config.resize(sccs.size());

        // The scheduler owns its read policy: hand the node-id -> SCC table to
        // the resolver so it can stamp intra-SCC edges StartTime (sub-step
        // sampling) and cross-SCC edges Latest (sequential DAG, Gauss-Seidel
        // semantics).
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
        set_resolver(std::make_shared<ssp4sim::scheduling::La2DataAccessResolver>(raw_nodes(), std::move(scc_of)));

        // Config read once here; run_loop() only multiplies the cached fraction
        // by the macro duration, never re-reads config.
        min_substep_fraction = utils::Config::getOr(
            "simulation.executor.la2.min_substep_fraction", 0.001);

        auto configured_iters = utils::Config::getOr(
            "simulation.executor.la2.iterations", -1);

        // Per-loop sub-step schedule template, resolved once here so run_loop()
        // does not re-read config on every macro step. The Factor recipe honors
        // BOTH the iteration count (fixed count of shrinking sub-steps landing on
        // the macro end) and the shrink factor; the min-substep fraction is kept
        // as a macro-relative value and applied to each step in run_loop.
        substep::Config schedule_template;
        const auto mode = utils::Config::getOr(
            "simulation.executor.la2.mode", std::string("linear"));
        if (mode == "factor" || mode == "geometric")
        {
            // "geometric" is the legacy alias for the factor/shrinking mode.
            schedule_template.mode = substep::Mode::Factor;
            schedule_template.factor = utils::Config::getOr(
                "simulation.executor.la2.factor", 0.8);
            schedule_template.max_steps = static_cast<std::size_t>(utils::Config::getOr(
                "simulation.executor.la2.max_steps", 64));
        }
        else
        {
            // "linear" is the new name; "fixed" is the legacy alias. Both mean
            // equal pre-selected sub-steps, count = iteration count.
            schedule_template.mode = substep::Mode::Linear;
        }
        for (std::size_t i = 0; i < sccs.size(); ++i)
        {
            loop_config[i] = schedule_template;
            if (is_loop[i])
            {
                // A loop group is any SCC that is_loop(): a multi-node SCC, or
                // a single node with a self-edge (feedback self-reference).
                loop_iterations[i] = configured_iters > 0 ? configured_iters : sccs[i].size();
                loop_config[i].steps = loop_iterations[i];
            }
        }

        LOG_INFO(log, "[{func}] La2Scheduler: {n} SCCs, execution order has {m} steps",
                  __func__, sccs.size(), execution_order.size());
    }

    std::string La2Scheduler::to_string() const
    {
        std::ostringstream oss;
        oss << "La2Scheduler:\n";
        oss << "  SCCs: " << sccs.size() << "\n";
        oss << "  Execution steps: " << execution_order.size() << "\n";
        for (std::size_t i = 0; i < execution_order.size(); ++i)
        {
            auto idx = execution_order[i];
            auto &comp = sccs[idx];
            oss << "  Step " << i << " (SCC #" << idx << ", " << comp.size()
                << " nodes):\n";
            for (auto *node : comp)
            {
                oss << "    - " << node->name << "\n";
            }
        }
        return oss.str();
    }

    // =========================================================================
    //  Execution - walk the condensed DAG
    // =========================================================================

    uint64_t La2Scheduler::invoke(StepData step_data)
    {
        for (auto idx : execution_order)
        {
            // Dispatch on the cached loop flag — not raw size — so that a
            // single-node SCC with a self-edge (a genuine feedback loop) also
            // takes the relaxation path instead of being run once like an
            // acyclic node.
            if (is_loop[idx])
            {
                run_loop(idx, step_data);
            }
            else
            {
                run_sequential(idx, step_data);
            }
        }
        return step_data.end_time;
    }

    void La2Scheduler::run_sequential(std::size_t scc_idx, const StepData &step)
    {
        auto s = StepData(step.start_time, step.end_time);
        for (auto *node : sccs[scc_idx])
        {
            node->invoke(s);
        }
    }

    // =========================================================================
    //  Loop relaxation
    // =========================================================================

    void La2Scheduler::run_loop(std::size_t scc_idx, const StepData &step)
    {
        auto &comp = sccs[scc_idx];

        auto cfg = loop_config[scc_idx];
        if (cfg.mode == substep::Mode::Factor && cfg.steps == 0)
        {
            // Free-shrink only: the min-substep threshold is macro-relative, so
            // it is resolved against this step's duration here. Fixed-count
            // Factor keeps the precomputed template verbatim.
            const auto macro_dt = step.end_time - step.start_time;
            cfg.min_substep = static_cast<uint64_t>(std::llround(
                static_cast<double>(macro_dt) * min_substep_fraction));
        }

        auto schedule = substep::build_substep_schedule(step.start_time, step.end_time, cfg);

        IF_LOG({
            LOG_DEBUG(log, "[{func}] Loop SCC #{idx} ({size} nodes, "
                           "{iters} iters, mode={mode}, {steps} sub-steps)",
                      __func__, scc_idx, comp.size(), loop_iterations[scc_idx], int(cfg.mode), schedule.size());
        });

        for (auto &[sub_start, sub_end] : schedule)
        {
            auto s = StepData(sub_start, sub_end);
            // Intra-SCC edges are stamped StartTime, so each sub-step reads the
            // previous sub-step's commitments (deterministic relaxation); the
            // shared invoke_group_parallel kernel parallels the group members.
            invoke_group_parallel(comp, s);
        }
    }

}