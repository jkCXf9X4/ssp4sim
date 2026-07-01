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
                ssp4cpp::utils::log::make_logger("ssp4sim.analysis.SspSystemBuilder");
            return logger;
        }

        std::string get_full_path(std::string name, std::string prefix = "")
        {
            return prefix.empty() ? name : prefix + "." + name;
        }

        // std::map<std::string, SspConnector *> get_system_connector_map(SspSystem *system, std::string prefix = "")
        // {
        //     std::map<std::string, SspConnector *> map;
        //     for (auto &sub_system : system->nested_systems)
        //     {
        //         auto nested_map = get_system_connector_map(sub_system.get(), get_full_path(system->name, prefix));
        //         map.merge(nested_map);
        //     }
        //     for (auto &model : system->models)
        //     {
        //         auto model_map = get_connector_map(model.get(), prefix);
        //         map.merge(model_map);
        //     }
        //     return map;
        // }

        // std::map<std::string, SspConnector *> get_connector_map(SspModel *model, std::string prefix = "")
        // {
        //     std::map<std::string, SspConnector *> map;
        //     for (auto &connector : model->connectors)
        //     {
        //         auto key = get_full_path(connector->name, prefix);
        //         map[key] = connector.get();
        //     }
        //     return map;
        // }

        // void override(std::map<std::string, SspConnector *> map, std::map<std::string, std::unique_ptr<ext::ParameterValue>> start_value_map)
        // {
        //     for (auto &[name, start_value] : start_value_map)
        //     {
        //         if (map.contains(name))
        //         {
        //             map[name]->initial_value = std::make_unique<ext::ParameterValue>(start_value);
        //         }
        //     }
        // }

        // /// Recursively walk the SSD tree DFS post-order and apply parameter overrides.
        // /// Children first (subsystems, then components), then system-level overrides.
        // void apply_overrides_in_system(
        //     ssp4sim::analysis::SspSystem *system,
        //     const std::string &prefix = "")
        // {

        //     for (auto &sub_system : system->nested_systems)
        //     {
        //         apply_overrides_in_system(sub_system.get(), get_full_path(system->name, prefix));
        //     }

        //     for (auto &model : system->models)
        //     {
        //         auto connector_map = get_connector_map(model.get(), "");
        //         override(connector_map, model->parameter_bindings);
        //     }

        //     auto system_connector_map = get_system_connector_map(system, prefix);
        //     override(system_connector_map, system->parameter_bindings);
        // }

        template <typename T, typename U>
        T *create(U item)
        {
            auto wrapper = std::make_unique<T>(item);
            node_owner.emplace_back(std::move(wrapper));
            return wrapper.get();
        }

        SspSystemNode build_tree(const ssp4sim::analysis::SspSystem system)
        {
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

    SspSystemNode SspTreeBuilder::build(const SspSystem system)
    {
        node_owner.reserve(1000);

        LOG_TRACE_L1(log(), "[{func}] Building SspSystem from SSP", __func__);

        system_tree = build_tree(system);
        LOG_INFO(log(), "[{}] Tree: {}", __func__, system_tree.get_tree())

        LOG_TRACE_L1(log(), "[{func}] Applied SSP parameter overrides", __func__);
        LOG_WARNING(log(), "[{func}] Parametersets not applied!", __func__);
        // apply_overrides_in_system(system_tree);

        LOG_TRACE_L1(log(), "[{func}] exit", __func__);
        return system_tree;
    }

} // namespace ssp4sim::analysis
