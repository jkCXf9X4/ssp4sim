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
        RealtimeMacroExecutor(std::vector<std::shared_ptr<Invocable>> nodes);

        uint64_t invoke(StepData step_data) override;

        uint64_t realtime_start_reference = 0;
        uint64_t macro_step = 0;

    private:
        void wait_for_realtime_sync(uint64_t simulation_time);
    };

}
