#pragma once

#include "execution/seidel/seidel_base.hpp"

#include <vector>

namespace ssp4sim::graph
{
    class ParallelSeidel final : public SeidelBase
    {
    public:
        Logger log = Logger("ssp4sim.execution.ParallelSeidel", LogLevel::info);

        ParallelSeidel(std::vector<Invocable *> nodes);

        virtual void print(std::ostream &os) const
        {
            os << "ParallelSeidel:\n{}\n";
        }

        /**
         * Traverse the connection graph and invoke nodes when all parents have been invoked for this timestep.
         * [hot path]
         */
        uint64_t invoke(StepData step_data) override final;
    };
}
