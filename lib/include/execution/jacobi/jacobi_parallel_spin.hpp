#pragma once

#include "cutecpp/log.hpp"

#include "execution/jacobi/jacobi_base.hpp"

#include "task_thread_pool2.hpp"

#include <cstdint>
#include <vector>

namespace ssp4sim::graph
{
    class JacobiParallelSpin final : public ExecutionBase
    {
    public:
        Logger log = Logger("ssp4sim.execution.JacobiParallelSpin", LogLevel::info);

        utils::ThreadPool2 pool;

        JacobiParallelSpin(std::vector<Invocable *> nodes, int threads);

        uint64_t invoke(StepData step_data) override final;
    };
}
