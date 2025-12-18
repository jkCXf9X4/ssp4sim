#pragma once


#include "execution/jacobi/jacobi_base.hpp"

#include "task_thread_pool.hpp"

#include "cutecpp/log.hpp"

#include <cstdint>
#include <future>
#include <vector>

namespace ssp4sim::graph
{
    class JacobiParallelFutures final : public ExecutionBase
    {
    public:
        Logger log = Logger("ssp4sim.execution.JacobiParallelFutures", LogLevel::info);

        utils::ThreadPool pool;
        std::vector<std::future<void>> futures;

        JacobiParallelFutures(std::vector<Invocable *> nodes, int threads);

        uint64_t invoke(StepData step_data) override final;
    };
}
