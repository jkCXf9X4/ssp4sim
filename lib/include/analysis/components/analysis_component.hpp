#pragma once

#include "ssp4sim_definitions.hpp"

#include <cstdint>
#include <string>

namespace ssp4sim::analysis
{

    class AnalysisComponent : public virtual types::IWritable
    {
    public:
        std::string name;

        AnalysisComponent() = default;
        ~AnalysisComponent() = default;

        std::string to_string() const
        {
            return this->name + ":\n{}\n";
        }
    };
} // namespace ssp4sim::analysis