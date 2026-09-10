#pragma once


#include "execution/jacobi/jacobi_base.hpp"

#include "task_thread_pool.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <cstdint>
#include <future>
#include <memory>
#include <vector>

namespace ssp4sim::graph
{
    class JacobiParallelFutures final : public ExecutionBase
    {
    public:
        utils::ThreadPool pool;
        std::vector<std::future<void>> futures;

        JacobiParallelFutures(std::vector<std::shared_ptr<Invocable>> nodes, int threads);

        uint64_t invoke(StepData step_data) override final;
    };
}