#include "execution/loop_aware/la2_scheduler.hpp"

#include "execution/substep/substep_schedule.hpp"

#include "config.hpp"

#include <cmath>
#include <cstdint>
#include <memory>
#include <sstream>
#include <string>
#include <utility>
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
    }

    // =========================================================================
    //  Construction
    // =========================================================================

    La2Scheduler::La2Scheduler(std::vector<Invocable *> nodes)
        : ExecutionBase(nodes),
          graph(nodes),
          sccs(graph.sccs),
          execution_order(graph.execution_order),
          log(ssp4cpp::utils::log::make_logger("ssp4sim.execution.La2Scheduler"))
    {
        this->name = "La2Scheduler";
        graph.analyze();

        // Create JacobiParallelTBB instances for loop SCC groups.
        loop_executors.resize(sccs.size());
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
            // group is dispatched to run_loop and therefore needs a loop
            // executor; keep this predicate consistent with invoke().
            if (graph.groups[i]->is_loop())
            {
                loop_iterations[i] = configured_iters > 0 ? configured_iters : sccs[i].size();
                loop_executors[i] = std::make_unique<JacobiParallelTBB>(sccs[i]);
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
        auto &comp = sccs[scc_idx];
        auto s = StepData(step.start_time,
                          step.end_time,
                          step.end_time - step.start_time,
                          step.start_time,
                          step.end_time);
        for (auto *node : comp)
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
        auto *loop_exec = loop_executors[scc_idx].get();

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
            // legacy loop_aware default (0.8 at refactor HEAD) and the value
            // documented in docs/configuration.md.
            cfg.factor = getConfigOrLegacy<double>(
                "simulation.executor.la2.factor",
                "simulation.executor.loop_aware.factor", 0.8);
            cfg.steps = loop_iterations[scc_idx];
            const auto macro_dt = step.end_time - step.start_time;
            // min_substep_fraction / max_steps are NEW keys (no legacy
            // equivalent existed); they only constrain the free-shrink path
            // (steps == 0), which "geometric" does not use. Read them through
            // the same fallback for consistency.
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
            // equal pre-selected sub-steps, count = iteration count (the
            // legacy "fixed" semantics: macro step split into n equal
            // sub-steps, sub_dt = macro_dt / n). Any other legacy string also
            // fell through to the fixed/equal behavior.
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
            auto s = StepData(sub_start, sub_end, sub_end - sub_start, sub_start, sub_end);
            loop_exec->invoke(s);
        }
    }

}