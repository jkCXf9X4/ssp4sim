#pragma once

#include "ssp4cpp/utils/log.hpp"

#include "execution/jacobi/jacobi_base.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace ssp4sim::graph
{

    class JacobiParallelTBB final : public ExecutionBase
    {
    public:
        JacobiParallelTBB(std::vector<std::shared_ptr<Invocable>> nodes);

        uint64_t invoke(StepData step_data) override final;
    };

}