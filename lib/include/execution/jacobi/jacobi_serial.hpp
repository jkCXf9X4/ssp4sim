#pragma once

#include "execution/jacobi/jacobi_base.hpp"

#include "cutecpp/log.hpp"

namespace ssp4sim::graph
{
    class JacobiSerial final : public JacobiBase
    {
    public:
        Logger log = Logger("ssp4sim.execution.JacobiSerial", LogLevel::info);

        JacobiSerial(std::vector<Invocable *> nodes);

        void print(std::ostream &os) const override
        {
            os << "JacobiSerial:\n{}\n";
        }

        uint64_t invoke(StepData step_data) override final;
    };
}
