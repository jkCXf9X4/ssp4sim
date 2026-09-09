#pragma once

#include "ssp4sim_definitions.hpp"

#include "utils/primitives/node.hpp"

#include <cstdint>
#include <string>

// Rename to execution
namespace ssp4sim::graph
{

    class StepData : public types::IWritable
    {
    public:
        uint64_t start_time;
        uint64_t end_time;
        uint64_t timestep;

        StepData();
        ~StepData() = default;

        StepData(uint64_t start_time,
                 uint64_t end_time);

        std::string to_string() const override;
    };

    enum class TemporalType : int
    {
        Algebraic,           // Instantaneous, no delay
        Explicit,            // modeled
        PartiallyImplicitly, // partially modeled
        FullyImplicitly      // Not modeled
    };

    class Invocable : public utils::graph::Node
    {
    public:
        uint64_t walltime_ns = 0; // measurement of runtime
        uint64_t id = 0;

        TemporalType temporal_type = TemporalType::Algebraic;
        uint64_t delay = 0;

        uint64_t current_time = 0;

        Invocable()
            : id(next_id++)
        {
        }

        virtual ~Invocable() = default;

        virtual void enter_init();
        virtual void exit_init();

        // Simple wrapper for enter/exit init
        virtual void init();

        virtual uint64_t invoke(StepData data) = 0;

        std::string to_string() const override;

        inline static uint64_t next_id = 0;
    };
}
