#pragma once

#include "ssp4sim_definitions.hpp"

#include "utils/node.hpp"

#include <cstdint>
#include <string>

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

        StepData() {}

        StepData(uint64_t start_time,
                 uint64_t end_time,
                 uint64_t timestep)
        {
            this->start_time = start_time;
            this->end_time = end_time;
            this->timestep = timestep;
            this->input_time = this->start_time;
            this->output_time = this->end_time;
        }

        StepData(uint64_t start_time,
                 uint64_t end_time,
                 uint64_t timestep,
                 uint64_t input_time,
                 uint64_t output_time)
        {
            this->start_time = start_time;
            this->end_time = end_time;
            this->timestep = timestep;
            this->input_time = input_time;
            this->output_time = output_time;
        }

        virtual void print(std::ostream &os) const
        {
            os << "StepData: \n{"
               << " start_time: " << start_time
               << " end_time: " << end_time
               << " timestep: " << timestep
               << " input_time: " << input_time
               << " output_time: " << output_time;
            os << " }\n";
        }
    };

    enum class TemporalType : int
    {
        Algebraic,           // Instantaneous, no delay
        Explicit,            // modeled
        PartiallyImplicitly, // partially modeled
        FullyImplicitly      // Not modeled
    };

    using enum TemporalType;

    class Invocable : public utils::graph::Node, public virtual types::IPrintable
    {
    public:
        uint64_t walltime_ns = 0;
        uint64_t id = 0;

        TemporalType temporal_type = TemporalType::Algebraic;
        uint64_t delay = 0;

        uint64_t current_time = 0;

        virtual void enter_init() {}
        virtual void exit_init() {}

        // simple wrapper for when enter and exit is not needed
        virtual void init()
        {
            enter_init();
            exit_init();
        }

        virtual uint64_t invoke(StepData data) = 0;

        virtual void print(std::ostream &os) const
        {
            os << "Invocable:\n{}\n";
        }
    };
}