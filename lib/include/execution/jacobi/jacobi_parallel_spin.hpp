#pragma once

#include "ssp4cpp/utils/log.hpp"

#include "execution/jacobi/jacobi_base.hpp"

#include "task_thread_pool2.hpp"

#include <cstdint>
#include <vector>

namespace ssp4sim::graph
{
    class JacobiParallelSpin final : public ExecutionBase
    {
    public:
        quill::Logger* log = ssp4cpp::utils::log::make_logger("ssp4sim.execution.JacobiParallelSpin", quill::LogLevel::TraceL1);

        utils::ThreadPool2 pool;

        JacobiParallelSpin(std::vector<Invocable *> nodes, int threads);

        uint64_t invoke(StepData step_data) override final;
    };
}
