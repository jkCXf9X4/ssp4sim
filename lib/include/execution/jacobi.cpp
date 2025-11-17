#include "execution/jacobi.hpp"

#include "executor_utils.hpp"

#include <execution>
#include <exception>
#include <future>
#include <mutex>
#include <utility>

namespace ssp4sim::graph
{

    JacobiSerial::JacobiSerial(std::vector<Invocable *> nodes) : ExecutionBase(nodes)
    {
        log(info)("[{}] ", __func__);
    }

    uint64_t JacobiSerial::invoke(StepData step_data)
    {
        auto step = StepData(step_data.start_time, step_data.end_time, step_data.timestep);

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

    JacobiParallelFutures::JacobiParallelFutures(std::vector<Invocable *> nodes, int threads)
        : ExecutionBase(nodes), pool(threads)
    {
        log(info)("[{}] JacobiParallelFutures", __func__);
    }

    uint64_t JacobiParallelFutures::invoke(StepData step_data)
    {
        IF_LOG({
            log(debug)("[{}] stepdata: {}", __func__, step_data.to_string());
        });

        auto step = StepData(step_data.start_time, step_data.end_time, step_data.timestep);

        for (auto &node : nodes)
        {
            futures.push_back(pool.enqueue([&]()
                                           { node->invoke(step); }));
        }
        for (auto &f : futures)
        {
            f.get();
        }
        futures.clear();

        wait_for_result_collection();

        return step_data.end_time;
    }

    JacobiParallelTBB::JacobiParallelTBB(std::vector<Invocable *> nodes) : ExecutionBase(nodes)
    {
        log(info)("[{}] JacobiParallelTBB", __func__);
    }

    uint64_t JacobiParallelTBB::invoke(StepData step_data)
    {
        IF_LOG({
            log(debug)("[{}] stepdata: {}", __func__, step_data.to_string());
        });

        auto step = StepData(step_data.start_time, step_data.end_time, step_data.timestep);
        step.timestep = sub_step;

        std::exception_ptr captured_exception;
        std::mutex exception_mutex;

        std::for_each(std::execution::par, nodes.begin(), nodes.end(),
                      [&](auto &node)
                      {
                        try
                        {
                            invoke_sub_step(node, step, false);
                        }
                        catch (...)
                        {
                            std::scoped_lock lock(exception_mutex);
                            if (!captured_exception)
                            {
                                captured_exception = std::current_exception();
                            }
                        }
                      });

        wait_for_result_collection();

        if (captured_exception)
        {
            std::rethrow_exception(captured_exception);
        }

        return step_data.end_time;
    }

    JacobiParallelSpin::JacobiParallelSpin(std::vector<Invocable *> nodes, int threads)
        : ExecutionBase(nodes), pool(threads)
    {
        log(info)("[{}] JacobiParallelSpin", __func__);
    }

    uint64_t JacobiParallelSpin::invoke(StepData step_data)
    {
        IF_LOG({
            log(debug)("[{}] stepdata: {}", __func__, step_data.to_string());
        });

        auto step = StepData(step_data.start_time, step_data.end_time, step_data.timestep);

        pool.ready(static_cast<int>(nodes.size()));

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
            for (auto &done : pool.dones)
            {
                if (!done)
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

}
