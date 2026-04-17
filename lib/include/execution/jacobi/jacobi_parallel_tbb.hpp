#pragma once

#include "ssp4cpp/utils/log.hpp"

#include "execution/jacobi/jacobi_base.hpp"

#include <cstdint>
#include <vector>

namespace ssp4sim::graph
{

    class JacobiParallelTBB final : public ExecutionBase
    {
    public:
        quill::Logger* log = ssp4cpp::utils::log::make_logger("ssp4sim.execution.JacobiParallelTBB", quill::LogLevel::TraceL1);

        JacobiParallelTBB(std::vector<Invocable *> nodes);

        uint64_t invoke(StepData step_data) override final;
    };

}
