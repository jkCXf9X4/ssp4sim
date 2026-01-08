#pragma once

#include "execution/jacobi/jacobi_base.hpp"

#include "cutecpp/log.hpp"

#include <cstdint>
#include <vector>

namespace ssp4sim::graph
{
    class JacobiSerial final : public JacobiBase
    {
    public:
        Logger log = Logger("ssp4sim.execution.JacobiSerial", LogLevel::info);

        JacobiSerial(std::vector<Invocable *> nodes);

        std::string to_string() const override
        {
            return "JacobiSerial:\n{}\n";
        }

        uint64_t invoke(StepData step_data) override final;
    };
}
