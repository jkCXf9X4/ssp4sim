#pragma once

#include "execution/seidel/seidel_base.hpp"

#include <cstdint>
#include <vector>

namespace ssp4sim::graph
{

    class SerialSeidel final : public SeidelBase
    {
    public:
        quill::Logger* log = ssp4cpp::utils::log::make_logger("ssp4sim.execution.SerialSeidel", quill::LogLevel::TraceL1);

        SerialSeidel(std::vector<Invocable *> nodes);

        std::string to_string() const override
        {
            return "SerialSeidel:\n{}\n";
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
