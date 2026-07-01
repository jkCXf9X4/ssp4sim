#include "tree_builder.hpp"

#include "../1_ssp_parser/schema_extensions/SSP1_SystemStructureDescription_Ext.hpp"
#include "../1_ssp_parser/schema_extensions/SSP1_SystemStructureParameter_Ext.hpp"
#include "../1_ssp_parser/schema_extensions/SSP_Ext.hpp"
#include "../1_ssp_parser/schema_extensions/FMI2_modelDescription_Ext.hpp"
#include "../1_ssp_parser/schema_extensions/FMI2_Enums_Ext.hpp"
#include "utils/time/time.hpp"

#include "ssp4cpp/ssp.hpp"

#include <memory>
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
    }

    SspSystemNode *SspTreeBuilder::build(SspSystem *analysis_system)
    {
        node_owner.reserve(1000);

        LOG_TRACE_L1(log(), "[{func}] Building tree from SspSystem", __func__);

        system_tree = build_tree(analysis_system, node_owner);

        LOG_INFO(log(), "[{func}] Tree built with {} nodes", __func__, node_owner.size());

        // TODO: implement parameter set application
        // apply_parameters(system_tree, analysis_system);
        LOG_WARNING(log(), "[{func}] Parametersets not applied!", __func__);

        LOG_TRACE_L1(log(), "[{func}] exit", __func__);
        return system_tree;
    }

} // namespace ssp4sim::analysis