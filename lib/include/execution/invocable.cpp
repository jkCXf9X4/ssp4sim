#include "execution/invocable.hpp"

#include <sstream>

namespace ssp4sim::graph
{

    StepData::StepData() = default;

    StepData::StepData(uint64_t start_time, uint64_t end_time, uint64_t timestep)
    {
        this->start_time = start_time;
        this->end_time = end_time;
        this->timestep = timestep;
        this->input_time = this->start_time;
        this->output_time = this->end_time;
    }

    StepData::StepData(uint64_t start_time,
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

    std::string StepData::to_string() const
    {
        std::ostringstream oss;
        oss << "StepData: \n{"
            << " start_time: " << start_time
            << " end_time: " << end_time
            << " timestep: " << timestep
            << " input_time: " << input_time
            << " output_time: " << output_time
            << " }\n";
        return oss.str();
    }

    void Invocable::enter_init() {}

    void Invocable::exit_init() {}

    void Invocable::init()
    {
        enter_init();
        exit_init();
    }

    std::string Invocable::to_string() const
    {
        return "Invocable:\n{}\n";
    }

}
