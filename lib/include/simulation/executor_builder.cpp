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

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

namespace ssp4sim::graph
{

    std::string ExecutorBuilder::to_string() const
    {
        return "ExecutorBuilder:\n{}\n";
    }

    std::shared_ptr<ExecutorBase> ExecutorBuilder::build(std::vector<std::shared_ptr<Invocable>> nodes)
    {
        // wrap in Macro executor

        // Each concrete executor constructs and wires its own read-path
        // resolver in its constructor (see ExecutorBase::set_resolver). The
        // builder only selects the executor family from config.

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
            specialized_executor = std::make_shared<graph::La2Scheduler>(nodes);
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

        if (utils::Config::getOr("simulation.realtime", false))
        {
            return std::make_shared<graph::RealtimeMacroExecutor>(
                std::vector<std::shared_ptr<Invocable>>{specialized_executor});
        }
        else
        {
            return std::make_shared<graph::MacroExecutor>(
                std::vector<std::shared_ptr<Invocable>>{specialized_executor});
        }
    }

}