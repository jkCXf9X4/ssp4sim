#include "executor_builder.hpp"

#include "executor/custom/custom_executors.hpp"

#include "executor/jacobi/jacobi_parallel_fut.hpp"
#include "executor/jacobi/jacobi_parallel_spin.hpp"
#include "executor/jacobi/jacobi_parallel_tbb.hpp"
#include "executor/jacobi/jacobi_serial.hpp"

#include "executor/seidel/seidel_serial.hpp"
#include "executor/seidel/seidel_parallel.hpp"
#include "executor/loop_aware/la2_builder.hpp"
#include "executor/substep/macro_substep_executor.hpp"

#include "resolver/data_access_resolver.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace ssp4sim::graph
{

    ExecutorBuilder::ExecutorBuilder(const ssp4sim::ExecutorOptions &options,
                                     uint64_t macro_step)
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.execution.ExecutorBuilder")),
          options_(options),
          macro_step_(macro_step)
    {
        // Every concrete executor registers as one leaf variant: a `selects`
        // predicate over the typed options and a uniform `(nodes) -> executor`
        // factory. build() resolves the single matching variant from the
        // config set — no per-family branching. The flat resolver (Jacobi /
        // custom delay: StartTime; Seidel: EndTime) is derived and installed
        // here, on the assembly side — the executor constructors stay
        // resolver-free (mirrors make_la2_stack).

        register_variant("jacobi_serial", [](const auto &o)
                         { return o.method == "jacobi" && !o.jacobi_parallel; }, [this](std::vector<std::shared_ptr<Invocable>> nodes)
                         {
                             install_flat_resolver(nodes, ssp4sim::scheduling::AccessMode::StartTime);
                             LOG_INFO(log, "[builder] Executor: JacobiSerial");
                             return std::make_shared<JacobiSerial>(std::move(nodes)); });

        register_variant("jacobi_parallel_tbb", [](const auto &o)
                         { return o.method == "jacobi" && o.jacobi_parallel && o.jacobi_method == 1; }, [this](std::vector<std::shared_ptr<Invocable>> nodes)
                         {
                             install_flat_resolver(nodes, ssp4sim::scheduling::AccessMode::StartTime);
                             LOG_INFO(log, "[builder] Executor: JacobiParallelTBB");
                             return std::make_shared<JacobiParallelTBB>(std::move(nodes)); });

        register_variant("jacobi_parallel_spin", [](const auto &o)
                         { return o.method == "jacobi" && o.jacobi_parallel && o.jacobi_method == 2; }, [this](std::vector<std::shared_ptr<Invocable>> nodes)
                         {
                             install_flat_resolver(nodes, ssp4sim::scheduling::AccessMode::StartTime);
                             LOG_INFO(log, "[builder] Executor: JacobiParallelSpin");
                             return std::make_shared<JacobiParallelSpin>(std::move(nodes), options_.workers); });

        register_variant("jacobi_parallel_futures", [](const auto &o)
                         { return o.method == "jacobi" && o.jacobi_parallel && o.jacobi_method == 3; }, [this](std::vector<std::shared_ptr<Invocable>> nodes)
                         {
                             install_flat_resolver(nodes, ssp4sim::scheduling::AccessMode::StartTime);
                             LOG_INFO(log, "[builder] Executor: JacobiParallelFutures");
                             return std::make_shared<JacobiParallelFutures>(std::move(nodes), options_.workers); });

        register_variant("seidel_serial", [](const auto &o)
                         { return o.method == "seidel" && !o.seidel_parallel; }, [this](std::vector<std::shared_ptr<Invocable>> nodes)
                         {
                             // Seidel (Gauss-Seidel) runs upstream-to-downstream
                             // within a step; consumers read producers' current-step
                             // output (end time).
                             install_flat_resolver(nodes, ssp4sim::scheduling::AccessMode::EndTime);
                             LOG_INFO(log, "[builder] Executor: SerialSeidel");
                             return std::make_shared<SerialSeidel>(std::move(nodes)); });

        register_variant("seidel_parallel", [](const auto &o)
                         { return o.method == "seidel" && o.seidel_parallel; }, [this](std::vector<std::shared_ptr<Invocable>> nodes)
                         {
                            throw std::runtime_error(
                                 "Executor method 'parallel_seidel' requests ParallelSeidel, "
                                 "which is NOT implemented. "
                                 "Use 'seidel' (serial) or 'jacobi' instead.");

                             install_flat_resolver(nodes, ssp4sim::scheduling::AccessMode::EndTime);
                             LOG_INFO(log, "[builder] Executor: ParallelSeidel");
                             return std::make_shared<ParallelSeidel>(std::move(nodes)); });

        register_variant("custom_delay", [](const auto &o)
                         { return o.method == "custom_delay"; }, [this](std::vector<std::shared_ptr<Invocable>> nodes)
                         {
                             install_flat_resolver(nodes, ssp4sim::scheduling::AccessMode::StartTime);
                             LOG_INFO(log, "[builder] Executor: DelayExecutor");
                             return std::make_shared<DelayExecutor>(std::move(nodes)); });

        register_variant("custom_delay_partial", [](const auto &o)
                         { return o.method == "custom_delay_partial"; }, [this](std::vector<std::shared_ptr<Invocable>> nodes)
                         {
                             install_flat_resolver(nodes, ssp4sim::scheduling::AccessMode::StartTime);
                             LOG_INFO(log, "[builder] Executor: DelayExecutorPartial");
                             return std::make_shared<DelayExecutorPartial>(std::move(nodes)); });

        // "loop_aware" is the legacy name (see docs/configuration.md and
        // resources/loop_aware_nested*.json); "la2" is the refactored name.
        // Both are accepted by the single la2 variant and dispatch to the same
        // loop-aware stack factory (which derives and installs its own
        // stack-wide resolver).
        register_variant("la2", [](const auto &o)
                         { return o.method == "la2" || o.method == "loop_aware"; }, [this](std::vector<std::shared_ptr<Invocable>> nodes)
                         {
                             LOG_INFO(log, "[builder] Executor: La2");
                             return make_la2_stack(std::move(nodes), options_.la2); });
    }

    void ExecutorBuilder::register_variant(
        std::string name,
        std::function<bool(const ssp4sim::ExecutorOptions &)> selects,
        Factory factory)
    {
        variants_.push_back(Variant{std::move(name), std::move(selects), std::move(factory)});
    }

    std::string ExecutorBuilder::to_string() const
    {
        return "ExecutorBuilder:\n{}\n";
    }

    std::shared_ptr<ExecutorBase> ExecutorBuilder::build(std::vector<std::shared_ptr<Invocable>> nodes)
    {
        // Uniform dispatch: the config set decides which registered variant
        // applies; resolve it (exactly one must match), build the specialized
        // executor (la2's factory assembles the whole nested stack), then wrap
        // in a MacroExecutor sized from the configured timestep.

        std::vector<const Variant *> matches;
        matches.reserve(variants_.size());
        for (const auto &variant : variants_)
        {
            if (variant.selects(options_))
            {
                matches.push_back(&variant);
            }
        }

        if (matches.empty())
        {
            throw std::runtime_error("No executor variant matches config: method='" +
                                     options_.method + "'");
        }
        if (matches.size() > 1)
        {
            std::string names;
            for (const auto *match : matches)
            {
                if (!names.empty())
                    names += ", ";
                names += match->name;
            }
            throw std::runtime_error("Ambiguous executor config: method='" +
                                     options_.method + "' matches variants: " + names);
        }

        auto specialized_executor = matches[0]->factory(std::move(nodes));

        // Always wrap in a MacroExecutor; `realtime` records whether the outer
        // macro steps are paced to the wall clock.
        return std::make_shared<graph::MacroExecutor>(
            std::vector<std::shared_ptr<Invocable>>{specialized_executor},
            macro_step_, options_.realtime);
    }

}