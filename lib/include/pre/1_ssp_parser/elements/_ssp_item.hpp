#pragma once

#include "ssp4sim_definitions.hpp"
#include "utils/node.hpp"

#include <cstdint>
#include <string>

namespace ssp4sim::analysis
{
    enum class SspItemType
    {
        Connector,
        Connection,
        ModelVariable,
        Model,
        System
    };

    constexpr std::string_view to_string(SspItemType type)
    {
        switch (type)
        {
        case SspItemType::Connector:
            return "Connector";
        case SspItemType::Connection:
            return "Connection";
        case SspItemType::ModelVariable:
            return "ModelVariable";
        case SspItemType::Model:
            return "Model";
        case SspItemType::System:
            return "System";
        }

        return "Unknown";
    }

    class SspItem : public virtual types::IWritable
    {
    public:
        std::string name;
        SspItemType type;

        SspItem() = default;
        ~SspItem() = default;

        std::string to_string() const override
        {
            return this->name + ":\n{}\n";
        }
    };

} // namespace ssp4sim::analysis