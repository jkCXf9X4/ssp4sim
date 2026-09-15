#include "executor_builder.hpp"

#include "executor/custom/custom_executors.hpp"

#include "executor/jacobi/jacobi_parallel_fut.hpp"
#include "executor/jacobi/jacobi_parallel_spin.hpp"
#include "executor/jacobi/jacobi_parallel_tbb.hpp"
#include "executor/jacobi/jacobi_serial.hpp"

#include "executor/seidel/seidel_serial.hpp"
#include "executor/seidel/seidel_parallel.hpp"
#include "executor/loop_aware/la2_builder.hpp"
#include "executor/macro/macro_executor.hpp"
#include "executor/macro/realtime_macro_executor.hpp"

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
        // Each strategy is a uniform `(nodes) -> executor` factory over the
        // builder's typed options; the dispatch in build() is a plain map
        // lookup on options_.method.

        builders_["jacobi"] = [this](std::vector<std::shared_ptr<Invocable>> nodes)
            -> std::shared_ptr<ExecutorBase>
        {
            // Jacobi samples every producer at the sub-step start. The resolver
            // is derived and installed here, on the assembly side — the
            // executor constructors stay resolver-free (mirrors make_la2_stack).
            install_flat_resolver(nodes, ssp4sim::scheduling::AccessMode::StartTime);

            if (options_.jacobi_parallel)
            {
                if (options_.jacobi_method == 1)
                {
                    LOG_INFO(log, "[builder] Executor: JacobiParallelTBB");
                    return std::make_shared<JacobiParallelTBB>(std::move(nodes));
                }
                else if (options_.jacobi_method == 2)
                {
                    LOG_INFO(log, "[builder] Executor: JacobiParallelSpin");
                    return std::make_shared<JacobiParallelSpin>(std::move(nodes), options_.workers);
                }
                else if (options_.jacobi_method == 3)
                {
                    LOG_INFO(log, "[builder] Executor: JacobiParallelFutures");
                    return std::make_shared<JacobiParallelFutures>(std::move(nodes), options_.workers);
                }
                else
                {
                    throw std::runtime_error("Unknown parallelization method");
                }
            }
            else
            {
                LOG_INFO(log, "[builder] Executor: JacobiSerial");
                return std::make_shared<JacobiSerial>(std::move(nodes));
            }
        };

        builders_["seidel"] = [this](std::vector<std::shared_ptr<Invocable>> nodes)
            -> std::shared_ptr<ExecutorBase>
        {
            // Seidel (Gauss-Seidel) runs upstream-to-downstream within a step;
            // consumers read producers' current-step output (end time).
            install_flat_resolver(nodes, ssp4sim::scheduling::AccessMode::EndTime);

            if (options_.seidel_parallel)
            {
                LOG_INFO(log, "[builder] Executor: ParallelSeidel");
                return std::make_shared<ParallelSeidel>(std::move(nodes));
            }
            else
            {
                LOG_INFO(log, "[builder] Executor: SerialSeidel");
                return std::make_shared<SerialSeidel>(std::move(nodes));
            }
        };

        builders_["custom_delay"] = [this](std::vector<std::shared_ptr<Invocable>> nodes)
            -> std::shared_ptr<ExecutorBase>
        {
            install_flat_resolver(nodes, ssp4sim::scheduling::AccessMode::StartTime);
            LOG_INFO(log, "[builder] Executor: DelayExecutor");
            return std::make_shared<DelayExecutor>(std::move(nodes));
        };

        builders_["custom_delay_partial"] = [this](std::vector<std::shared_ptr<Invocable>> nodes)
            -> std::shared_ptr<ExecutorBase>
        {
            install_flat_resolver(nodes, ssp4sim::scheduling::AccessMode::StartTime);
            LOG_INFO(log, "[builder] Executor: DelayExecutorPartial");
            return std::make_shared<DelayExecutorPartial>(std::move(nodes));
        };

        // "loop_aware" is the legacy name (see docs/configuration.md and
        // resources/loop_aware_nested*.json); "la2" is the refactored name.
        // Both dispatch to the same loop-aware stack factory.
        builders_["la2"] = [this](std::vector<std::shared_ptr<Invocable>> nodes)
            -> std::shared_ptr<ExecutorBase>
        {
            LOG_INFO(log, "[builder] Executor: La2");
            return make_la2_stack(std::move(nodes), options_.la2);
        };
        builders_["loop_aware"] = builders_["la2"];

        // ParallelSeidel is NOT implemented: its invoke() throws "This is not
        // implemented" at runtime. Fail at dispatch time with a clear,
        // actionable message instead of silently mis-dispatching or falling
        // through to the generic "Unknown executor method".
        builders_["parallel_seidel"] = [](std::vector<std::shared_ptr<Invocable>>)
            -> std::shared_ptr<ExecutorBase>
        {
            throw std::runtime_error(
                "Executor method 'parallel_seidel' requests ParallelSeidel, "
                "which is NOT implemented. "
                "Use 'seidel' (serial) or 'jacobi' instead.");
        };
        builders_["parallel-seidel"] = builders_["parallel_seidel"];
    }

    std::string ExecutorBuilder::to_string() const
    {
        return "ExecutorBuilder:\n{}\n";
    }

    std::shared_ptr<ExecutorBase> ExecutorBuilder::build(std::vector<std::shared_ptr<Invocable>> nodes)
    {
        // Uniform dispatch: lookup the method strategy, build the specialized
        // executor (la2's factory assembles the whole nested stack), then wrap
        // in a MacroExecutor sized from the configured timestep.

        auto factory = builders_.find(options_.method);
        if (factory == builders_.end())
        {
            throw std::runtime_error("Unknown executor method: '" + options_.method + "'");
        }

        auto specialized_executor = factory->second(std::move(nodes));

        if (options_.realtime)
        {
            return std::make_shared<graph::RealtimeMacroExecutor>(
                std::vector<std::shared_ptr<Invocable>>{specialized_executor},
                macro_step_);
        }
        else
        {
            return std::make_shared<graph::MacroExecutor>(
                std::vector<std::shared_ptr<Invocable>>{specialized_executor},
                macro_step_);
        }
    }

}