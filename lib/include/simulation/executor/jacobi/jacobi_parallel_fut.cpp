#include "executor/jacobi/jacobi_parallel_fut.hpp"

#include "resolver/data_access_resolver.hpp"

namespace ssp4sim::graph
{
    JacobiParallelFutures::JacobiParallelFutures(std::vector<std::shared_ptr<Invocable>> nodes, int threads)
        : ExecutorBase(nodes, "ssp4sim.execution.JacobiParallelFutures"),
          pool(threads)
    {
        set_resolver(std::make_shared<ssp4sim::scheduling::DataAccessResolver>(raw_nodes(),
                                                                               ssp4sim::scheduling::AccessMode::StartTime));
        LOG_INFO(log, "[{func}] JacobiParallelFutures", __func__);
    }

    uint64_t JacobiParallelFutures::invoke(StepData step_data)
    {
        IF_LOG({
            LOG_DEBUG(log, "[{func}] stepdata: {stepdata}", __func__, step_data.to_string());
        });

        auto step = StepData(step_data.start_time, step_data.end_time);

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

        return step_data.end_time;
    }
}