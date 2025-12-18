#include "execution/jacobi/jacobi_parallel_spin.hpp"

#include "executor_utils.hpp"

#include <execution>
#include <exception>
#include <future>
#include <mutex>
#include <utility>

namespace ssp4sim::graph
{

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
