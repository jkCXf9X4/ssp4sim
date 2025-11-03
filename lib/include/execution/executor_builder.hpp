#pragma once

#include "cutecpp/log.hpp"

#include "ssp4sim_definitions.hpp"

#include "executor.hpp"
#include "config.hpp"

#include "jacobi.hpp"
#include "seidel.hpp"
#include "custom_executors.hpp"

#include <assert.h>
#include <execution>

namespace ssp4sim::graph 
{

    class ExecutorBuilder : public types::IPrintable
    {
    public:
        Logger log = Logger("ssp4sim.execution.Delay", LogLevel::info);

        ExecutorBuilder()
        {
        }

        virtual void print(std::ostream &os) const
        {
            os << "ExecutorBuilder:\n{}\n";
        }

        std::unique_ptr<ExecutionBase> build(std::vector<Invocable *> nodes)
        {
            auto executor_method = utils::Config::getOr<std::string>("simulation.executor.method", "jacobi");
            int workers = utils::Config::getOr<int>("simulation.executor.thread_pool_workers", 5);

            if (executor_method == "jacobi")
            {
                if (utils::Config::getOr<bool>("simulation.executor.jacobi.parallel", false))
                {
                    int parallel_method = utils::Config::getOr<int>("simulation.executor.jacobi.method", 1); // default TBB

                    if (parallel_method == 1)
                    {
                        log(info)("[{}] Executor: JacobiParallelTBB", __func__);
                        return std::make_unique<JacobiParallelTBB>(nodes);
                    }
                    else if (parallel_method == 2)
                    {
                        log(info)("[{}] Executor: JacobiParallelSpin", __func__);
                        return std::make_unique<JacobiParallelSpin>(nodes, workers);
                    }
                    else if (parallel_method == 3)
                    {
                        log(info)("[{}] Executor: JacobiParallelFutures", __func__);
                        return std::make_unique<JacobiParallelFutures>(nodes, workers);
                    }
                    else
                    {
                        throw std::runtime_error("Unknown parallelization method");
                    }
                }
                else
                {
                    log(info)("[{}] Executor: JacobiSerial", __func__);
                    return std::make_unique<JacobiSerial>(nodes);
                }
            }
            else if (executor_method == "seidel")
            {
                if (utils::Config::getOr<bool>("simulation.executor.seidel.parallel", false))
                {
                    log(info)("[{}] Executor: ParallelSeidel", __func__);
                    return std::make_unique<ParallelSeidel>(nodes);
                }
                else
                {
                    log(info)("[{}] Executor: SerialSeidel", __func__);
                    return std::make_unique<SerialSeidel>(nodes);
                }
            }
            else if (executor_method == "custom_delay")
            {
                log(info)("[{}] Executor: DelayExecutor", __func__);
                return std::make_unique<DelayExecutor>(nodes);
            }
            else
            {
                throw std::runtime_error("Unknown executor method");
            }
        }
    };

}
