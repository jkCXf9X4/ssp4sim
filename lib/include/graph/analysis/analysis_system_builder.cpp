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
                ssp4cpp::utils::log::make_logger("ssp4sim.analysis.AnalysisSystemBuilder");
            return logger;
        }

        std::string get_full_path(std::string name, std::string prefix = "")
        {
            return prefix.empty() ? name : prefix + "." + name;
        }

        std::map<std::string, AnalysisConnector *> get_system_connector_map(AnalysisSystem *system, std::string prefix = "")
        {
            std::map<std::string, AnalysisConnector *> map;
            for (auto &sub_system : system->nested_systems)
            {
                auto nested_map = get_system_connector_map(sub_system.get(), get_full_path(system->name, prefix));
                map.merge(nested_map);
            }
            for (auto &model : system->models)
            {
                auto model_map = get_connector_map(model.get(), prefix);
                map.merge(model_map);
            }
            return map;
        }

        std::map<std::string, AnalysisConnector *> get_connector_map(AnalysisModel *model, std::string prefix = "")
        {
            std::map<std::string, AnalysisConnector *> map;
            for (auto &connector : model->connectors)
            {
                auto key = get_full_path(connector->name, prefix);
                map[key] = connector.get();
            }
            return map;
        }

        void override(std::map<std::string, AnalysisConnector *> map, std::map<std::string, std::unique_ptr<ext::ParameterValue>> start_value_map)
        {
            for (auto &[name, start_value] : start_value_map)
            {
                if (map.contains(name))
                {
                    map[name]->initial_value = std::make_unique<ext::ParameterValue>(start_value);
                }
            }
        }

        /// Recursively walk the SSD tree DFS post-order and apply parameter overrides.
        /// Children first (subsystems, then components), then system-level overrides.
        void apply_overrides_in_system(
            ssp4sim::analysis::AnalysisSystem *system,
            const std::string &prefix = "")
        {

            for (auto &sub_system : system->nested_systems)
            {
                apply_overrides_in_system(sub_system.get(), get_full_path(system->name, prefix));
            }

            for (auto &model : system->models)
            {
                auto connector_map = get_connector_map(model.get(), "");
                override(connector_map, model->parameter_bindings);
            }

            auto system_connector_map = get_system_connector_map(system, prefix);
            override(system_connector_map, system->parameter_bindings);
        }



        SystemNode *build_tree(ssp4sim::analysis::AnalysisSystem *system)
        {
            auto system_node = create_node(system, tree.system_nodes);

            for (auto &sub_sys : system->nested_systems)
            {
                auto n = build_tree(sub_sys.get());
                system_node.add_child(n);
            }

            for (auto &model : system->models)
            {
                auto model_node = create_node(model.get(), tree.model_nodes);
                system_node.add_child(model_node);

                for (auto &m_connector : model_node->connectors)
                {
                    auto model_connector_node = create_node(m_connector.get(), tree.connector_nodes);
                    model_node.add_child(model_connector_node);
                }

                for (auto &m_variable : model_node->model_variables)
                {
                    auto model_var_node = create_node(m_variable.get(), tree.variable_nodes);
                    model_node.add_child(model_var_node);
                }
            }

            for (auto &m_connector : system->connectors)
            {
                auto connector_node = create_node(m_connector.get(), tree.connector_nodes);
                system_node.add_child(connector_node);
            }

            for (auto &connection : system->connections)
            {
                auto connection_node = create_node(connection.get(), tree.connection_nodes);
                system_node.add_child(connection_node);
            }
            return (system_node);
        }
    }

    std::unique_ptr<AnalysisSystem> AnalysisSystemBuilder::build(ssp4cpp::Ssp *ssp)
    {

        LOG_TRACE_L1(log(), "[{func}] Building AnalysisSystem from SSP", __func__);

        analysis_system = std::make_unique<AnalysisSystem>(ssp->ssd->System);
        
        tree.top_system = build_tree(analysis_system.get());

        apply_overrides_in_system(analysis_system.get(), "");

        LOG_INFO(log(), "[{}] Tree: {}", __func__, tree.top_system.get_tree())

        LOG_TRACE_L1(log(), "[{func}] Applied SSP parameter overrides", __func__);

        LOG_TRACE_L1(log(), "[{func}] exit", __func__);
        return analysis_system;
    }

} // namespace ssp4sim::analysis
