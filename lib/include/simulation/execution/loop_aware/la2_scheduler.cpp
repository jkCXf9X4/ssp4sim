#include "execution/loop_aware/la2_scheduler.hpp"

#include "execution/substep/substep_schedule.hpp"

#include "config.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <execution>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace ssp4sim::graph
{

    namespace
    {
        // Backward-compatible config read: prefer the refactored `la2.*` key,
        // fall back to the legacy `loop_aware.*` key, then the built-in
        // default. Type-mismatch errors propagate exactly as before (via
        // Config::getOr), and resolvePath() is used only to detect presence.
        template <typename T, typename Default>
        T getConfigOrLegacy(const std::string &la2_key,
                            const std::string &legacy_key,
                            Default fallback)
        {
            if (utils::Config::resolvePath(la2_key) != nullptr)
            {
                return utils::Config::getOr(la2_key, fallback);
            }
            if (utils::Config::resolvePath(legacy_key) != nullptr)
            {
                return utils::Config::getOr(legacy_key, fallback);
            }
            return fallback;
        }

        // The analysis operates on raw pointers borrowed from the owned nodes.
        // Called from the member-init list after ExecutionBase has populated
        // `nodes`.
        std::vector<Invocable *> raw_pointers(const std::vector<std::shared_ptr<Invocable>> &nodes)
        {
            std::vector<Invocable *> raw;
            raw.reserve(nodes.size());
            for (const auto &node : nodes)
            {
                raw.push_back(node.get());
            }
            return raw;
        }
    }

    // =========================================================================
    //  Construction
    // =========================================================================

    La2Scheduler::La2Scheduler(std::vector<std::shared_ptr<Invocable>> nodes)
        : ExecutionBase(nodes, "ssp4sim.execution.La2Scheduler"),
          graph(raw_pointers(this->nodes))
    {
        this->name = "La2Scheduler";

        graph.analyze();

        // Copy the SCC / execution-order tables so the scheduler stays movable
        // and never aliases members of `graph` transitively.
        sccs = graph.sccs;
        execution_order = graph.execution_order;
        loop_iterations.resize(sccs.size(), 1);

        // Backward-compatible: prefer `simulation.executor.la2.iterations`,
        // fall back to the legacy `simulation.executor.loop_aware.iterations`.
        auto configured_iters = getConfigOrLegacy<int>(
            "simulation.executor.la2.iterations",
            "simulation.executor.loop_aware.iterations", -1);
        for (std::size_t i = 0; i < sccs.size(); ++i)
        {
            // A loop group is any SCC that is_loop(): a multi-node SCC, or a
            // single node with a self-edge (feedback self-reference). Such a
            // group is dispatched to run_loop; keep this predicate consistent
            // with invoke().
            if (graph.groups[i]->is_loop())
            {
                loop_iterations[i] = configured_iters > 0 ? configured_iters : sccs[i].size();
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
            // Dispatch on SccGroup::is_loop() — not raw size — so that a
            // single-node SCC with a self-edge (a genuine feedback loop) also
            // takes the relaxation path instead of being run once like an
            // acyclic node.
            if (graph.groups[idx]->is_loop())
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

        substep::Config cfg;
        // Backward-compatible reads: la2.* first, then legacy loop_aware.*.
        auto mode = getConfigOrLegacy<std::string>(
            "simulation.executor.la2.mode",
            "simulation.executor.loop_aware.mode", std::string("linear"));
        if (mode == "factor" || mode == "geometric")
        {
            // "geometric" is the legacy alias for the factor/shrinking mode.
            cfg.mode = substep::Mode::Factor;
            // Legacy "geometric" honored BOTH the iteration count (number of
            // sub-steps) and the shrink factor together. substep::Mode::Factor
            // with steps > 0 replays exactly that: `steps` shrinking sub-steps
            // whose boundaries follow frac(k) = (1 - r^k)/(1 - r^steps),
            // landing exactly on the macro end. Default factor 0.8 matches the
            // legacy loop_aware default.
            cfg.factor = getConfigOrLegacy<double>(
                "simulation.executor.la2.factor",
                "simulation.executor.loop_aware.factor", 0.8);
            cfg.steps = loop_iterations[scc_idx];
            const auto macro_dt = step.end_time - step.start_time;
            cfg.min_substep = static_cast<uint64_t>(std::llround(
                static_cast<double>(macro_dt) * getConfigOrLegacy<double>(
                    "simulation.executor.la2.min_substep_fraction",
                    "simulation.executor.loop_aware.min_substep_fraction", 0.001)));
            cfg.max_steps = static_cast<std::size_t>(getConfigOrLegacy<int>(
                "simulation.executor.la2.max_steps",
                "simulation.executor.loop_aware.max_steps", 64));
        }
        else
        {
            // "linear" is the new name; "fixed" is the legacy alias. Both mean
            // equal pre-selected sub-steps, count = iteration count.
            cfg.mode = substep::Mode::Linear;
            cfg.steps = loop_iterations[scc_idx];
        }

        auto schedule = substep::build_substep_schedule(step.start_time, step.end_time, cfg);

        IF_LOG({
            LOG_DEBUG(log, "[{func}] Loop SCC #{idx} ({size} nodes, "
                           "{iters} iters, mode={mode}, {steps} sub-steps)",
                      __func__, scc_idx, comp.size(), loop_iterations[scc_idx], mode, schedule.size());
        });

        for (auto &[sub_start, sub_end] : schedule)
        {
            auto s = StepData(sub_start, sub_end);
            std::for_each(std::execution::par, comp.begin(), comp.end(),
                          [&](auto *node)
                          {
                              node->invoke(s);
                          });
        }
    }

}