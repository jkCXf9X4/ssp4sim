#pragma once

#include "executor.hpp"

#include <cstdint>
#include <map>
#include <string>

namespace ssp4sim::graph
{

    /// Realtime-aware macro-step executor.
    class RealtimeMacroExecutor final : public ExecutionBase
    {
    public:

        RealtimeMacroExecutor(std::map<std::string, Invocable *> node_map);

        uint64_t invoke(StepData step_data) override;

        uint64_t realtime_start_reference = 0;
        uint64_t macro_step = 0;
    };

}

