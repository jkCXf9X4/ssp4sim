#pragma once

#include "execution/seidel/seidel_base.hpp"

#include <vector>

namespace ssp4sim::graph
{

    class SerialSeidel final : public SeidelBase
    {
    public:
        Logger log = Logger("ssp4sim.execution.SerialSeidel", LogLevel::info);

        SerialSeidel(std::vector<Invocable *> nodes);

        void print(std::ostream &os) const override
        {
            os << "SerialSeidel:\n{}\n";
        }

        // some idea that this might be more effective than looping over all items
        // Not used at the moment
        void invoke_node(SeidelNode &node, StepData step_data);

        /**
         * Traverse the connection graph and invoke nodes when all parents have been invoked for this timestep.
         * [hot path]
         */
        uint64_t invoke(StepData step_data) override final;
    };
}
