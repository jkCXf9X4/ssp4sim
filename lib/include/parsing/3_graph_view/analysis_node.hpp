#pragma once

#include "ssp4sim_definitions.hpp"
#include "utils/node.hpp"

#include <cstdint>
#include <string>
#include <stdexcept>
#include <format>

namespace ssp4sim::analysis
{

    /// Typed transient graph node wrapping an analysis data object.

    /// T is the source type (AnalysisModel, AnalysisConnector, AnalysisConnection,
    /// or AnalysisModelVariable).
    template <std::derived_from<AnalysisComponent> T>
    struct AnalysisNode : public utils::graph::Node
    {
        /// The source object this node wraps (non-owning).
        T *source = nullptr;

        explicit AnalysisNode(const std::string &name_, T *source_ = nullptr)
            : Node(name_), source(source_)
        {
        }

        template <std::derived_from<AnalysisComponent> U>
        U *as()
        {
            if (auto *p = dynamic_cast<U *>(source))
                return p;

            throw std::runtime_error(
                std::format("Node '{}' cannot be cast from {}",
                            name,
                            to_string(source->type)));
        }
    };

    /// Convenience aliases for the typed specializations.
    using ModelNode = AnalysisNode<AnalysisModel>;
    using ConnectorNode = AnalysisNode<AnalysisConnector>;
    using ConnectionNode = AnalysisNode<AnalysisConnection>;
    using VariableNode = AnalysisNode<AnalysisModelVariable>;
    using SystemNode = AnalysisNode<AnalysisSystem>;

} // namespace ssp4sim::analysis