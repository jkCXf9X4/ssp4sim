#pragma once

#include "ssp4cpp/utils/log.hpp"

#include "invocable.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ssp4sim::graph
{
    class ExecutorBase : public Invocable
    {
    public:
        ssp4cpp::utils::log::Logger *log = nullptr;

        // Realtime pacing: when true, invoke() sleeps until the wall clock
        // reaches `realtime_start_reference + step_start` before each emitted
        // step, so simulation time tracks real time from construction. The
        // reference is captured at construction time, not per invoke().
        const bool realtime = false;

        // shared ownership: executors may wrap each other and the graph may
        // outlive a single executor without move gymnastics
        std::vector<std::shared_ptr<Invocable>> nodes = {};

        // Read policy intentionally lives OUTSIDE the executor: the assembler
        // (ExecutorBuilder's factories / make_la2_stack) derives the resolver
        // and installs it on the models via install_resolver() /
        // install_flat_resolver() (resolver/data_access_resolver.hpp).
        // Constructors are resolver-free.

        ExecutorBase() = default;

        ExecutorBase(std::shared_ptr<Invocable> node,
                     std::string log_name = "ssp4sim.execution.ExecutorBase",
                     const bool realtime = false);

        ExecutorBase(std::vector<std::shared_ptr<Invocable>> nodes,
                     std::string log_name = "ssp4sim.execution.ExecutorBase",
                     const bool realtime = false);

        void init() override;

    protected:
        bool single_node = false;

        // Wall-clock epoch (ns) captured at construction when realtime pacing
        // is enabled; each step [s, e) waits for reference + s before running.
        // Unused (0) when realtime == false.
        uint64_t realtime_start_reference = 0;

        // Sleep until realtime_start_reference + simulation_time when realtime
        // pacing is enabled; no-op otherwise.
        void wait_for_realtime_sync(uint64_t simulation_time);

        std::string to_string() const
        {
            return "ExecutorBase: " + this->name + ":\n{}\n";
        }
    };
}
