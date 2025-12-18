#pragma once

#include "cutecpp/log.hpp"

#include "execution/jacobi/jacobi_base.hpp"

#include <cstdint>
#include <vector>

namespace ssp4sim::graph
{

    class JacobiParallelTBB final : public ExecutionBase
    {
    public:
        Logger log = Logger("ssp4sim.execution.JacobiParallelTBB", LogLevel::info);

        JacobiParallelTBB(std::vector<Invocable *> nodes);

        uint64_t invoke(StepData step_data) override final;
    };

}
