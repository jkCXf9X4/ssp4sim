#include "execution/jacobi/jacobi_parallel_fut.hpp"

#include "executor_utils.hpp"

namespace ssp4sim::graph
{
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
}
