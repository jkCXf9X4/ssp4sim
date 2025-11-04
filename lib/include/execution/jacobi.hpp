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

        JacobiSerial(std::vector<Invocable *> nodes);

        void print(std::ostream &os) const override;

        uint64_t invoke(StepData step_data) override final;
    };

    class JacobiParallelFutures final : public ExecutionBase
    {
    public:
        Logger log = Logger("ssp4sim.execution.JacobiParallelFutures", LogLevel::info);

        utils::ThreadPool pool;
        std::vector<std::future<void>> futures;

        JacobiParallelFutures(std::vector<Invocable *> nodes, int threads);

        uint64_t invoke(StepData step_data) override final;
    };

    class JacobiParallelTBB final : public ExecutionBase
    {
    public:
        Logger log = Logger("ssp4sim.execution.JacobiParallelTBB", LogLevel::info);

        JacobiParallelTBB(std::vector<Invocable *> nodes);

        uint64_t invoke(StepData step_data) override final;
    };

    class JacobiParallelSpin final : public ExecutionBase
    {
    public:
        Logger log = Logger("ssp4sim.execution.JacobiParallelSpin", LogLevel::info);

        utils::ThreadPool2 pool;

        JacobiParallelSpin(std::vector<Invocable *> nodes, int threads);

        uint64_t invoke(StepData step_data) override final;
    };
}
