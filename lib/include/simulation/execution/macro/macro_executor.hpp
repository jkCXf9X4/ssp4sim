#pragma once

#include "ssp4sim_definitions.hpp"

#include "executor.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>


namespace ssp4sim::graph
{

    class MacroExecutor final : public ExecutionBase
    {
    public:

        MacroExecutor(std::vector<Invocable *> nodes);

        uint64_t invoke(StepData step_data) override;

        uint64_t macro_step = 0;
    };

}

