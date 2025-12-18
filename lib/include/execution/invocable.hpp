#pragma once

#include "ssp4sim_definitions.hpp"

#include "utils/node.hpp"

#include <cstdint>
#include <ostream>

// Rename to execution
namespace ssp4sim::graph
{

    class StepData : public types::IPrintable
    {
    public:
        uint64_t start_time;
        uint64_t end_time;
        uint64_t timestep;
        uint64_t input_time = 0;
        uint64_t output_time = 0;

        bool use_input_time = false;
        bool use_output_time = false;

        StepData();

        StepData(uint64_t start_time,
                 uint64_t end_time,
                 uint64_t timestep);

        StepData(uint64_t start_time,
                 uint64_t end_time,
                 uint64_t timestep,
                 uint64_t input_time,
                 uint64_t output_time);

        void print(std::ostream &os) const override;
    };

    enum class TemporalType : int
    {
        Algebraic,           // Instantaneous, no delay
        Explicit,            // modeled
        PartiallyImplicitly, // partially modeled
        FullyImplicitly      // Not modeled
    };

    class Invocable : public utils::graph::Node, public virtual types::IPrintable
    {
    public:
        uint64_t walltime_ns = 0;
        uint64_t id = 0;

        TemporalType temporal_type = TemporalType::Algebraic;
        uint64_t delay = 0;

        uint64_t current_time = 0;

        virtual void enter_init();
        virtual void exit_init();

        // SImple wrapper for enter/exit init
        virtual void init();

        virtual uint64_t invoke(StepData data) = 0;

        void print(std::ostream &os) const override;
    };
}
