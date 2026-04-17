#pragma once

#include "ssp4cpp/utils/log.hpp"

#include "signal/recorder.hpp"

#include "invocable.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace ssp4sim::graph
{
    class ExecutionBase : public Invocable
    {
    public:
        quill::Logger* log = ssp4cpp::utils::log::make_logger("ssp4sim.execution.ExecutionBase", quill::LogLevel::TraceL1);
        // the executor should not own the nodes
        std::vector<Invocable *> nodes;

        signal::DataRecorder *recorder = nullptr;
        bool wait_for_recorder = false;

        uint64_t sub_step = 0;

        ExecutionBase(std::vector<Invocable *> nodes);

        void set_recorder(signal::DataRecorder *dr);

        void wait_for_result_collection();

        void init() override;

        std::string to_string() const
        {
            return this->name + ":\n{}\n";
        }
    };
}
