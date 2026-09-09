
#include "config.hpp"
#include "execution/executor_builder.hpp"

#include "execution/custom/custom_executors.hpp"

#include "execution/jacobi/jacobi_parallel_fut.hpp"
#include "execution/jacobi/jacobi_parallel_spin.hpp"
#include "execution/jacobi/jacobi_parallel_tbb.hpp"
#include "execution/jacobi/jacobi_serial.hpp"

#include "execution/seidel/seidel_serial.hpp"
#include "execution/seidel/seidel_parallel.hpp"
#include "execution/loop_aware/la2_scheduler.hpp"

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

    
    std::shared_ptr<ExecutionBase> ExecutorBuilder::build(std::vector<Invocable *> nodes, )
    {

        // wrap in Macro executor


        std::unique_ptr<ExecutionBase> specialized_executor;


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
                    specialized_executor = std::make_unique<JacobiParallelTBB>(nodes);
                }
                else if (parallel_method == 2)
                {
                    LOG_INFO(log, "[{func}] Executor: JacobiParallelSpin", __func__);
                    specialized_executor = std::make_unique<JacobiParallelSpin>(nodes, workers);
                }
                else if (parallel_method == 3)
                {
                    LOG_INFO(log, "[{func}] Executor: JacobiParallelFutures", __func__);
                    specialized_executor = std::make_unique<JacobiParallelFutures>(nodes, workers);
                }
                else
                {
                    throw std::runtime_error("Unknown parallelization method");
                }
            }
            else
            {
                LOG_INFO(log, "[{func}] Executor: JacobiSerial", __func__);
                specialized_executor = std::make_unique<JacobiSerial>(nodes);
            }
            specialized_executor.set_resolver(std::make_shared( **simple jacobi resolver**))
        }
        else if (executor_method == "seidel")
        {
            if (utils::Config::getOr("simulation.executor.seidel.parallel", false))
            {
                LOG_INFO(log, "[{func}] Executor: ParallelSeidel", __func__);
                specialized_executor = std::make_unique<ParallelSeidel>(nodes);
            }
            else
            {
                LOG_INFO(log, "[{func}] Executor: SerialSeidel", __func__);
                specialized_executor = std::make_unique<SerialSeidel>(nodes);
            }
            specialized_executor.set_resolver(std::make_shared( **simple seidel resolver**))
        }
        else if (executor_method == "custom_delay")
        {
            LOG_INFO(log, "[{func}] Executor: DelayExecutor", __func__);
            specialized_executor = std::make_unique<DelayExecutor>(nodes);
            specialized_executor.set_resolver(std::make_shared( **simple jacobi resolver**))
        }
        else if (executor_method == "custom_delay_partial")
        {
            LOG_INFO(log, "[{func}] Executor: DelayExecutorPartial", __func__);
            specialized_executor = std::make_unique<DelayExecutorPartial>(nodes);
            specialized_executor.set_resolver(std::make_shared( **simple jacobi resolver**))
        }
        else if (executor_method == "la2" || executor_method == "loop_aware")
        {
            // "loop_aware" is the legacy name (see docs/configuration.md and
            // resources/loop_aware_nested*.json); "la2" is the refactored name.
            // Both dispatch to the same scheduler.
            LOG_INFO(log, "[{func}] Executor: La2Scheduler", __func__);
            specialized_executor = std::make_unique<La2Scheduler>(nodes);
            specialized_executor.set_resolver(std::make_shared( **simple jacobi resolver**))
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


        if (config->realtime)
        {
            return std::make_unique<graph::RealtimeMacroExecutor>(specialized_executor);
        }
        else
        {
            return std::make_unique<graph::MacroExecutor>(specialized_executor);
        }


        throw std::runtime_error("Unknown executor method: '" + executor_method + "'");
    }

}
