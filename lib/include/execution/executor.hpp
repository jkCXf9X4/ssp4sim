#pragma once

#include "cutecpp/log.hpp"

#include "invocable.hpp"


namespace ssp4sim::utils
{
    class DataRecorder;
}

namespace ssp4sim::graph
{
    class ExecutionBase : public Invocable
    {
    public:
        Logger log = Logger("ssp4sim.execution.ExecutionBase", LogLevel::info);
        // the executor should not own the nodes
        std::vector<Invocable *> nodes;

        utils::DataRecorder *recorder = nullptr;
        bool wait_for_recorder = false;

        ExecutionBase(std::vector<Invocable *> nodes);

        void set_recorder(utils::DataRecorder *dr);

        void wait_for_result_collection();

        void init() override;
    };
}
