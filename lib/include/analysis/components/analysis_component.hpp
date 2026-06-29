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

    /// Typed transient graph node wrapping an analysis data object.
    /// Inherits from utils::graph::Node for Tarjan SCC compatibility.
    /// T is the source type (AnalysisModel, AnalysisConnector, AnalysisConnection,
    /// or AnalysisModelVariable).
    template <std::derived_from<AnalysisComponent> T>
    struct AnalysisNode : public utils::graph::Node
    {
        /// The source object this node wraps (non-owning).
        T *source = nullptr;
        ComponentType type;

        AnalysisNode() = default;

        explicit AnalysisNode(const std::string &name_, T *source_ = nullptr)
            : Node(name_), source(source_), type(source->type)
        {
        }

        std::unique_ptr<AnalysisNode<T>> clone() const
        {
            return std::make_unique<AnalysisNode<T>>(this->name, this->source);
        }
    };

} // namespace ssp4sim::analysis