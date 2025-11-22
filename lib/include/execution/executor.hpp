#pragma once

#include "cutecpp/log.hpp"

#include "signal/recorder.hpp"

#include "invocable.hpp"

namespace ssp4sim::graph
{
    class ExecutionBase : public Invocable
    {
    public:
        Logger log = Logger("ssp4sim.execution.ExecutionBase", LogLevel::info);
        // the executor should not own the nodes
        std::vector<Invocable *> nodes;

        signal::DataRecorder *recorder = nullptr;
        bool wait_for_recorder = false;

        uint64_t sub_step = 0;

        ExecutionBase(std::vector<Invocable *> nodes);

        void set_recorder(signal::DataRecorder *dr);

        void wait_for_result_collection();

        void init() override;

        virtual void print(std::ostream &os) const
        {
            os << this->name << ":\n{}\n";
        }
    };
}
