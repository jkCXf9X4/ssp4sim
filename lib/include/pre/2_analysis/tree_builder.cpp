#include "tree_builder.hpp"

#include "../1_ssp_parser/schema_extensions/SSP1_SystemStructureDescription_Ext.hpp"
#include "../1_ssp_parser/schema_extensions/SSP1_SystemStructureParameter_Ext.hpp"
#include "../1_ssp_parser/schema_extensions/SSP_Ext.hpp"
#include "../1_ssp_parser/schema_extensions/FMI2_modelDescription_Ext.hpp"
#include "../1_ssp_parser/schema_extensions/FMI2_Enums_Ext.hpp"
#include "utils/time/time.hpp"

#include "ssp4cpp/ssp.hpp"

#include <map>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

namespace ssp4sim::analysis
{

    namespace
    {

        ssp4cpp::utils::log::Logger *log()
        {
            static ssp4cpp::utils::log::Logger *logger =
                ssp4cpp::utils::log::make_logger("ssp4sim.analysis.SspTreeBuilder");
            return logger;
        }

        /// Build the tree of SspNode<T> wrappers from an SspSystem hierarchy.
        /// Each node holds a non-owning pointer into the SspSystem structure.
        /// Nodes are allocated into the owning vector so they outlive the
        /// call but do not own the underlying SspItem objects.
        template <typename T, typename U>
        T *create_node(std::vector<std::unique_ptr<utils::graph::Node>> &owner, U *item)
        {
            auto wrapper = std::make_unique<T>(item);
            auto *ptr = wrapper.get();
            owner.emplace_back(std::move(wrapper));
            return ptr;
        }

        SspSystemNode *build_tree(SspSystem *system,
                                  std::vector<std::unique_ptr<utils::graph::Node>> &owner)
        {
            auto *system_node = create_node<SspSystemNode>(owner, system);

            for (auto &sub_sys : system->nested_systems)
            {
                auto *n = build_tree(&sub_sys, owner);
                system_node->add_child(n);
            }

            for (auto &model : system->models)
            {
                auto *model_node = create_node<SspModelNode>(owner, &model);
                system_node->add_child(model_node);

                for (auto &m_connector : model.connectors)
                {
                    auto *conn_node = create_node<SspConnectorNode>(owner, &m_connector);
                    model_node->add_child(conn_node);
                }

                for (auto &m_variable : model.model_variables)
                {
                    auto *var_node = create_node<SspVariableNode>(owner, &m_variable);
                    model_node->add_child(var_node);
                }
            }

            for (auto &m_connector : system->connectors)
            {
                auto *conn_node = create_node<SspConnectorNode>(owner, &m_connector);
                system_node->add_child(conn_node);
            }

            for (auto &m_connection : system->connections)
            {
                auto *conn_node = create_node<SspConnectionNode>(owner, &m_connection);
                system_node->add_child(conn_node);
            }

            return system_node;
        }

        // -----------------------------------------------------------------------
        // Parameter set application
        // -----------------------------------------------------------------------

        /// Flatten all descendants of @p node into a map keyed by path segments.
        /// The node itself is not included.
        static void flatten_subtree(
            utils::graph::Node *node,
            std::map<std::vector<std::string>, utils::graph::Node *> &out,
            std::vector<std::string> prefix = {})
        {
            for (auto *child : node->children)
            {
                auto child_prefix = prefix;
                child_prefix.push_back(child->name);
                out.emplace(child_prefix, child);
                flatten_subtree(child, out, child_prefix);
            }
        }

        /// Apply a ParameterValue to a target node.
        /// Currently supports SspConnectorNode (sets initial_value).
        /// Other node types are logged at trace level and skipped.
        static void apply_value_to_node(
            utils::graph::Node *target,
            const ext::ParameterValue &param_value,
            const std::string &key)
        {
            if (auto *conn_node = dynamic_cast<SspConnectorNode *>(target))
            {
                conn_node->source->initial_value = param_value;
                LOG_TRACE_L1(log(), "[{func}] Applied parameter '{key}' to connector '{name}'",
                             __func__, key, conn_node->name);
            }
            else
            {
                LOG_TRACE_L1(log(), "[{func}] Parameter '{key}' matched node '{name}' but type not supported for value application",
                             __func__, key, target->name);
            }
        }

        /// Apply parameter bindings from @p node to its descendants.
        /// Uses post-order traversal (children before parent) so that lower-level
        /// bindings are applied first, matching SSP semantics.
        ///
        /// Parameter keys are dotted paths (e.g. "sine.amplitude" or "gain.k").
        /// Each key is matched as a suffix against the reconstructed dotted path
        /// of every descendant, with a dot boundary check to avoid false matches.
        static void apply_parameters(utils::graph::Node *node)
        {
            // Post-order: recurse into children first
            for (auto *child : node->children)
            {
                apply_parameters(child);
            }

            // Only SspSystemNode and SspModelNode carry parameter_bindings
            std::map<std::string, ext::ParameterValue> *bindings = nullptr;

            if (auto *sys_node = dynamic_cast<SspSystemNode *>(node))
            {
                bindings = &sys_node->source->parameter_bindings;
            }
            else if (auto *model_node = dynamic_cast<SspModelNode *>(node))
            {
                bindings = &model_node->source->parameter_bindings;
            }

            if (!bindings || bindings->empty())
            {
                return;
            }

            // Flatten all descendants of this node
            std::map<std::vector<std::string>, utils::graph::Node *> flat;
            flatten_subtree(node, flat);

            if (flat.empty())
            {
                return;
            }

            // Match each parameter key against flattened descendants
            for (const auto &[key, param_value] : *bindings)
            {
                bool matched = false;

                for (const auto &[path, target] : flat)
                {
                    // Reconstruct dotted path from segments
                    std::string dotted_path;
                    for (size_t i = 0; i < path.size(); ++i)
                    {
                        if (i > 0)
                            dotted_path += ".";
                        dotted_path += path[i];
                    }

                    // Suffix match with dot boundary:
                    // The key must match the end of dotted_path, and either
                    // the match covers the whole path or is preceded by a dot.
                    if (dotted_path.size() >= key.size() &&
                        dotted_path.compare(dotted_path.size() - key.size(), key.size(), key) == 0 &&
                        (dotted_path.size() == key.size() ||
                         dotted_path[dotted_path.size() - key.size() - 1] == '.'))
                    {
                        apply_value_to_node(target, param_value, key);
                        matched = true;
                    }
                }

                if (!matched)
                {
                    LOG_TRACE_L1(log(), "[{func}] Parameter '{key}' did not match any descendant",
                                 __func__, key);
                }
            }
        }
    }

    SspSystemNode *SspTreeBuilder::build(SspSystem *analysis_system)
    {
        node_owner.reserve(1000);

        LOG_TRACE_L1(log(), "[{func}] Building tree from SspSystem", __func__);

        system_tree = build_tree(analysis_system, node_owner);

        LOG_INFO(log(), "[{func}] Tree built with {} nodes", __func__, node_owner.size());

        // Apply parameter sets: post-order traversal matching parameter bindings
        // against flattened descendants using dotted-path suffix matching.
        apply_parameters(system_tree);
        LOG_INFO(log(), "[{func}] Parameter sets applied", __func__);

        LOG_TRACE_L1(log(), "[{func}] exit", __func__);
        return system_tree;
    }

} // namespace ssp4sim::analysis