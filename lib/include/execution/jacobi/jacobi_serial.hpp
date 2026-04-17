#pragma once

#include "execution/jacobi/jacobi_base.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <cstdint>
#include <vector>

namespace ssp4sim::graph
{
    class JacobiSerial final : public JacobiBase
    {
    public:
        ssp4cpp::utils::log::Logger* log = nullptr;

        JacobiSerial(std::vector<Invocable *> nodes);

        std::string to_string() const override
        {
            return "JacobiSerial:\n{}\n";
        }

        uint64_t invoke(StepData step_data) override final;
    };
}
