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
        ssp4cpp::utils::log::Logger* log = nullptr;

        JacobiParallelTBB(std::vector<Invocable *> nodes);

        uint64_t invoke(StepData step_data) override final;
    };

}
