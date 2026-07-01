#include "analysis/analysis_system_builder.hpp"

#include "SSP1_SystemStructureDescription_Ext.hpp"
#include "SSP1_SystemStructureParameter_Ext.hpp"
#include "SSP_Ext.hpp"
#include "FMI2_modelDescription_Ext.hpp"
#include "FMI2_Enums_Ext.hpp"
#include "utils/time.hpp"

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
                ssp4cpp::utils::log::make_logger("ssp4sim.analysis.SspGraphBuilder");
            return logger;
        }

        // create and place it in an owner vector
        // t will be discarded when the application dies
        template <typename T, typename U>
        T* create(U item)
        {
            auto wrapper = std::make_unique<T>(item)
            node_owner.emplace_back(std::move(wrapper));
            return wrapper.get();
        }


        SspSystemNode build_graph(const SspSystemNode *node, std::string prefix)
        {
            // map< path, node>  
            std::map<std::vector<std::string>, SspNode> 
            for (auto child : node->children)
            {
                if (auto p = dynamic_cast<SspSystemNode>(child))
                {
                    build_graph(p);
                }
            }

            auto system_node = create<SspSystemNode>(system);
            for (auto &sub_sys : system->nested_systems)
            {
                auto n = build_tree(sub_sys);
                system_node->add_child(n);
            }

            for (auto &model : system->models)
            {
                auto model_node = create<SspModelNode>(model);
                system_node->add_child(model_node);

                for (auto &m_connector : model_node->connectors)
                {
                    auto model_connector_node = create<SspConnectorNode>(m_connector);
                    model_node->add_child(model_connector_node);
                }

                for (auto &m_variable : model_node->model_variables)
                {
                    auto model_var_node = create<SspVariableNode>(m_variable);
                    model_node->add_child(model_var_node);
                }
            }

            for (auto &m_connector : system->connectors)
            {
                auto connector_node = create<SspConnectorNode>(m_connector);
                system_node->add_child(connector_node);
            }

            for (auto &m_connection : system->connections)
            {
                auto connection_node = create<SspConnectionNode>(m_connection);
                system_node->add_child(m_connection);
            }

            return (system_node);
        }
    }

    SspSystemNode SspGraphBuilder::build(const SspSystemNode *tree)
    {
        node_owner.reserve(1000);

        LOG_TRACE_L1(log(), "[{func}] Building SspSystem from SSP", __func__);
        
        system_tree = build_graph(tree);

        LOG_INFO(log(), "[{}] Graph: {}", __func__, "-")

        LOG_TRACE_L1(log(), "[{func}] exit", __func__);
        return system_tree;
    }

} // namespace ssp4sim::analysis
