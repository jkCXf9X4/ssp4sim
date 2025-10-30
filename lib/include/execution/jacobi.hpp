#pragma once

#include "cutecpp/log.hpp"

#include "ssp4sim_definitions.hpp"

#include "executor.hpp"
#include "invocable.hpp"

#include "task_thread_pool.hpp"
#include "task_thread_pool2.hpp"

#include "config.hpp"

#include <assert.h>
#include <execution>

namespace ssp4sim::graph
{

    class JacobiSerial final : public ExecutionBase
    {
    public:
        Logger log = Logger("ssp4sim.execution.JacobiSerial", LogLevel::info);

        const bool feedthrough = utils::Config::getOr<bool>("simulation.jacobi.feedthrough", false);

        JacobiSerial(std::vector<Invocable *> nodes) : ExecutionBase(nodes)
        {
            log(info)("[{}] Feedthrough {}", __func__, feedthrough);
        }

        virtual void print(std::ostream &os) const
        {
            os << "JacobiSerial:\n{}\n";
        }

        // hot path
        uint64_t invoke(StepData step_data) override final
        {
            auto step = StepData(step_data.start_time, step_data.end_time, step_data.timestep, step_data.start_time);

            IF_LOG({
                log(debug)("[{}] stepdata: {}", __func__, step_data.to_string());
            });

            for (auto &model : this->nodes)
            {
                model->invoke(step);
            }

            wait_for_result_collection();

            return step_data.end_time;
        }
    };

    class JacobiParallelFutures final : public ExecutionBase
    {
    public:
        Logger log = Logger("ssp4sim.execution.JacobiParallelFutures", LogLevel::info);

        utils::ThreadPool pool;
        std::vector<std::future<void>> futures;

        JacobiParallelFutures(std::vector<Invocable *> nodes, int threads) : ExecutionBase(nodes), pool(threads)
        {
            log(info)("[{}] JacobiParallelFutures", __func__);
        }

        // hot path
        uint64_t invoke(StepData step_data) override final
        {
            IF_LOG({
                log(debug)("[{}] stepdata: {}", __func__, step_data.to_string());
            });

            auto step = StepData(step_data.start_time, step_data.end_time, step_data.timestep, step_data.start_time);

            for (auto &node : nodes)
            {
                futures.push_back(pool.enqueue([&]()
                                               { node->invoke(step); }));
            }
            for (auto &f : futures)
                f.get();
            futures.clear();

            wait_for_result_collection();

            return step_data.end_time;
        }
    };

    class JacobiParallelTBB final : public ExecutionBase
    {
    public:
        Logger log = Logger("ssp4sim.execution.JacobiParallelTBB", LogLevel::info);

        JacobiParallelTBB(std::vector<Invocable *> nodes) : ExecutionBase(nodes)
        {
            log(info)("[{}] JacobiParallelTBB", __func__);
        }

        // hot path
        uint64_t invoke(StepData step_data) override final
        {
            IF_LOG({
                log(debug)("[{}] stepdata: {}", __func__, step_data.to_string());
            });

            auto step = StepData(step_data.start_time, step_data.end_time, step_data.timestep, step_data.start_time);

            std::for_each(std::execution::par, nodes.begin(), nodes.end(),
                          [&](auto &model)
                          {
                              model->invoke(step);
                          });

            wait_for_result_collection();

            return step_data.end_time;
        }
    };

    class JacobiParallelSpin final : public ExecutionBase
    {
    public:
        Logger log = Logger("ssp4sim.execution.JacobiParallelSpin", LogLevel::info);

        utils::ThreadPool2 pool;

        JacobiParallelSpin(std::vector<Invocable *> nodes, int threads) : ExecutionBase(nodes), pool(threads)
        {
            log(info)("[{}] JacobiParallelSpin", __func__);
        }

        // hot path
        uint64_t invoke(StepData step_data) override final
        {
            IF_LOG({
                log(debug)("[{}] stepdata: {}", __func__, step_data.to_string());
            });

            auto step = StepData(step_data.start_time, step_data.end_time, step_data.timestep, step_data.start_time);

            pool.ready(nodes.size());

            for (auto &node : nodes)
            {
                auto ti = utils::task_info{node, step};
                pool.enqueue(ti);
            }

            IF_LOG({
                log(info)("[{}] Spinning until all threads are done", __func__);
            });

            bool all_done = false;
            while (!all_done)
            {
                all_done = true;
                for (int i = 0; i < pool.dones.size(); ++i)
                {
                    if (!pool.dones[i])
                    {
                        all_done = false;
                        break;
                    }
                }
            }

            IF_LOG({
                log(info)("[{}] All threads completed", __func__);
            });

            wait_for_result_collection();
            
            return step_data.end_time;
        }
    };
}
