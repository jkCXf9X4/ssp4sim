#pragma once

#include "ssp4sim_definitions.hpp"
#include "utils/node.hpp"

#include <cstdint>
#include <string>

namespace ssp4sim::analysis
{
    enum class ComponentType
    {
        Connector,
        Connection,
        ModelVariable,
        Model,
        System
    };

    constexpr std::string_view to_string(ComponentType type)
    {
        switch (type)
        {
        case ComponentType::Connector:
            return "Connector";
        case ComponentType::Connection:
            return "Connection";
        case ComponentType::ModelVariable:
            return "ModelVariable";
        case ComponentType::Model:
            return "Model";
        case ComponentType::System:
            return "System";
        }

        return "Unknown";
    }

    class AnalysisComponent : public virtual types::IWritable
    {
    public:
        std::string name;
        ComponentType type;

        AnalysisComponent() = default;
        ~AnalysisComponent() = default;

        std::string to_string() const override
        {
            return this->name + ":\n{}\n";
        }
    };

} // namespace ssp4sim::analysis