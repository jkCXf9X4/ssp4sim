
#include "config.hpp"
#include "execution/executor_builder.hpp"

#include "execution/custom_executors.hpp"

#include "execution/jacobi/jacobi_parallel_fut.hpp"
#include "execution/jacobi/jacobi_parallel_spin.hpp"
#include "execution/jacobi/jacobi_parallel_tbb.hpp"
#include "execution/jacobi/jacobi_serial.hpp"

#include "execution/seidel/seidel_serial.hpp"
#include "execution/seidel/seidel_parallel.hpp"

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

    std::unique_ptr<ExecutionBase> ExecutorBuilder::build(std::vector<Invocable *> nodes)
    {
        auto executor_method = utils::Config::getOr("simulation.executor.method", std::string("jacobi"));
        int workers = utils::Config::getOr("simulation.executor.thread_pool_workers", 5);

        if (executor_method == "jacobi")
        {
            if (utils::Config::getOr("simulation.executor.jacobi.parallel", false))
            {
                int parallel_method = utils::Config::getOr("simulation.executor.jacobi.method", 1);

                if (parallel_method == 1)
                {
                    LOG_INFO(log, "[{}] Executor: JacobiParallelTBB", __func__);
                    return std::make_unique<JacobiParallelTBB>(nodes);
                }
                else if (parallel_method == 2)
                {
                    LOG_INFO(log, "[{}] Executor: JacobiParallelSpin", __func__);
                    return std::make_unique<JacobiParallelSpin>(nodes, workers);
                }
                else if (parallel_method == 3)
                {
                    LOG_INFO(log, "[{}] Executor: JacobiParallelFutures", __func__);
                    return std::make_unique<JacobiParallelFutures>(nodes, workers);
                }
                else
                {
                    throw std::runtime_error("Unknown parallelization method");
                }
            }
            else
            {
                LOG_INFO(log, "[{}] Executor: JacobiSerial", __func__);
                return std::make_unique<JacobiSerial>(nodes);
            }
        }
        else if (executor_method == "seidel")
        {
            if (utils::Config::getOr("simulation.executor.seidel.parallel", false))
            {
                LOG_INFO(log, "[{}] Executor: ParallelSeidel", __func__);
                return std::make_unique<ParallelSeidel>(nodes);
            }
            else
            {
                LOG_INFO(log, "[{}] Executor: SerialSeidel", __func__);
                return std::make_unique<SerialSeidel>(nodes);
            }
        }
        else if (executor_method == "custom_delay")
        {
            LOG_INFO(log, "[{}] Executor: DelayExecutor", __func__);
            return std::make_unique<DelayExecutor>(nodes);
        }
        else if (executor_method == "custom_delay_partial")
        {
            LOG_INFO(log, "[{}] Executor: DelayExecutorPartial", __func__);
            return std::make_unique<DelayExecutorPartial>(nodes);
        }

        throw std::runtime_error("Unknown executor method");
    }

}
