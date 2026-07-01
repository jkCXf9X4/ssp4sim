#pragma once

#include "ssp_item.hpp"

#include "utils/node.hpp"
#include "ssp4sim_definitions.hpp"

#include <cstdint>
#include <string>
#include <stdexcept>
#include <format>

namespace ssp4sim::analysis
{

    /// Typed transient graph node wrapping an analysis data object.

    template <std::derived_from<SspItem> T>
    struct SspNode : public utils::graph::Node
    {
        /// The source object this node wraps (owning).
        T *source;

        explicit SspNode(T *source_)
            : name(source_->name), source(source_)
        {
        }

        template <std::derived_from<SspItem> U>
        U *as()
        {
            if (auto p = dynamic_cast<U *>(source))
            {
                return p;
            }

            throw std::runtime_error(
                std::format("Node '{}' cannot be cast from {}",
                            name,
                            to_string(source->type)));
        }

        template <std::derived_from<SspItem> U>
        std::vector<U *> get_children_of_type()
        {
            std::vector<U *> out;
            for (auto child : children)
            {
                if (auto p = dynamic_cast<U *>(child))
                {
                    out.push_back(p);
                }
            }
            return out;
        }

        static std::map<std::vector<std::string>, SspNode *> flatten(SspNode *node, const std::vector<std::string> &prefix = {})
        {
            // map< path, node>
            std::map<std::vector<std::string>, SspNode *> out;
            for (auto *child : node->children)
            {
                auto child_prefix = prefix; // copy
                child_prefix.push_back(child->name);

                out.emplace(child_prefix, child);

                auto flat = flatten(child, child_prefix);
                out.merge(flat);
            }
            return out;
        }

        std::map<std::vector<std::string>, SspNode *> flatten()
        {
            return flatten(this);
        }
    };

    /// Convenience aliases for the typed specializations.
    using SspModelNode = SspNode<SspModel>;
    using SspConnectorNode = SspNode<SspConnector>;
    using SspConnectionNode = SspNode<Connection>;
    using SspVariableNode = SspNode<SspModelVariable>;
    using SspSystemNode = SspNode<SspSystem>;

} // namespace ssp4sim::analysis