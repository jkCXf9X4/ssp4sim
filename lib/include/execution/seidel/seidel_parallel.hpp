#pragma once

#include "execution/seidel/seidel_base.hpp"

#include <cstdint>
#include <vector>

namespace ssp4sim::graph
{
    class ParallelSeidel final : public SeidelBase
    {
    public:
        Logger log = Logger("ssp4sim.execution.ParallelSeidel", LogLevel::info);

        ParallelSeidel(std::vector<Invocable *> nodes);

        std::string to_string() const override
        {
            return "ParallelSeidel:\n{}\n";
        }

        /**
         * Traverse the connection graph and invoke nodes when all parents have been invoked for this timestep.
         * [hot path]
         */
        uint64_t invoke(StepData step_data) override final;
    };
}
