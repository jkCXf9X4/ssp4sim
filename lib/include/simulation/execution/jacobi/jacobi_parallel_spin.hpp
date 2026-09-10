#pragma once

#include "ssp4cpp/utils/log.hpp"

#include "execution/jacobi/jacobi_base.hpp"

#include "task_thread_pool2.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace ssp4sim::graph
{
    class JacobiParallelSpin final : public ExecutionBase
    {
    public:
        utils::ThreadPool2 pool;

        JacobiParallelSpin(std::vector<std::shared_ptr<Invocable>> nodes, int threads);

        uint64_t invoke(StepData step_data) override final;
    };
}