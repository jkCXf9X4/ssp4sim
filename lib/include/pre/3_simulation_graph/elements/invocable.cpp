#include "execution/invocable.hpp"

#include <sstream>

namespace ssp4sim::graph
{

    StepData::StepData() = default;

    StepData::StepData(uint64_t start_time, uint64_t end_time)
    {
        this->start_time = start_time;
        this->end_time = end_time;
        this->timestep = end_time - start_time;
    }

    std::string StepData::to_string() const
    {
        std::ostringstream oss;
        oss << "StepData: \n{"
            << " start_time: " << start_time
            << " end_time: " << end_time
            << " timestep: " << timestep
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
