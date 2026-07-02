#pragma once

#include "../../1_ssp_parser/elements/_ssp_item.hpp"
#include "../../1_ssp_parser/elements/ssp_system.hpp"
#include "../../1_ssp_parser/elements/ssp_model.hpp"
#include "../../1_ssp_parser/elements/ssp_connector.hpp"
#include "../../1_ssp_parser/elements/ssp_connection.hpp"
#include "../../1_ssp_parser/elements/ssp_model_variable.hpp"

#include "utils/primitives/node.hpp"
#include "ssp4sim_definitions.hpp"

#include <cstdint>
#include <map>
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
            : source(source_)
        {
            name = source_->name;
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
                            ssp4sim::analysis::to_string(source->type)));
        }

        /// Get children cast to a specific Node subclass (e.g. SspConnectorNode).
        template <typename U>
        std::vector<U *> get_child_nodes() const
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
    using SspConnectionNode = SspNode<SspConnection>;
    using SspVariableNode = SspNode<SspModelVariable>;
    using SspSystemNode = SspNode<SspSystem>;

} // namespace ssp4sim::analysis