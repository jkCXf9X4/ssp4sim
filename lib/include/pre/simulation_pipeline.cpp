#include "simulation_pipeline.hpp"

#include "pre/1_ssp_parser/ssp_parser.hpp"
#include "pre/1_ssp_parser/schema_extensions/FMI2_Enums_Ext.hpp"
#include "pre/2_analysis/tree_builder.hpp"
#include "pre/2_analysis/graph_builder.hpp"
#include "pre/3_simulation/sim_graph_builder.hpp"

#include "config.hpp"
#include "execution/invocable.hpp"
#include "signal/recorder.hpp"
#include "shared_config.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <cstdlib>
#include <fstream>
#include <functional>

namespace ssp4sim::pre
{

    SimulationPipelineResult build_simulation_graph(
        ssp4cpp::Ssp *ssp,
        ssp4sim::SharedConfig *config)
    {
        auto log = ssp4cpp::utils::log::make_logger("ssp4sim.pre.SimulationPipeline");

        LOG_INFO(log, "[{func}] - Creating analysis system", __func__);
        auto analysis_system = analysis::SspSystemBuilder().build(ssp);
        LOG_DEBUG(log, " -- analysis system built");

        LOG_INFO(log, "[{func}] - Building analysis tree", __func__);
        analysis::SspTreeBuilder tree_builder;
        auto *system_tree = tree_builder.build(&analysis_system);
        LOG_DEBUG(log, " -- analysis tree built");

        LOG_INFO(log, "[{func}] - Building analysis graph", __func__);
        analysis::SspGraphBuilder graph_builder;
        auto analysis_graph_data = graph_builder.build(system_tree);
        LOG_DEBUG(log, " -- analysis graph built");

        {
            LOG_DEBUG(log, " -- writing debug output files");
            std::ofstream(config->working_dir / "tree_output.txt") << system_tree->get_tree();

            std::vector<utils::graph::Node *> all_nodes;
            for (auto &n : analysis_graph_data.model_nodes)
                all_nodes.push_back(n.get());
            for (auto &n : analysis_graph_data.connector_nodes)
                all_nodes.push_back(n.get());
            for (auto &n : analysis_graph_data.connection_nodes)
                all_nodes.push_back(n.get());
            std::ofstream(config->working_dir / "graph_output.dot") << utils::graph::Node::to_dot(all_nodes);
        }

        // Write start_values.csv — log every connector with a valid initial value.
        // Traverse the tree to build qualified names that include subsystem prefixes.
        // Connector node names already include the parent model name (model.var).
        // For model nodes we pass the prefix unchanged; for system nodes we append
        // the subsystem name; for boundary connectors we use the system-qualified name.
        {
            auto start_value_log = std::ofstream(config->start_value_log_file, std::ios::out);
            std::function<void(utils::graph::Node *, std::string)> write_connectors;
            write_connectors = [&](utils::graph::Node *node, const std::string &prefix)
            {
                for (auto *child : node->children)
                {
                    if (auto *conn = dynamic_cast<analysis::SspConnectorNode *>(child))
                    {
                        auto *src = conn->source;
                        if (!src)
                            continue;
                        auto *raw = src->initial_value.raw_ptr();
                        if (!raw)
                            continue;
                        auto value_str = ext::fmi2::enums::data_type_to_string(src->data_type, raw);
                        std::string qualified =
                            prefix.empty() ? conn->name : prefix + "." + conn->name;
                        start_value_log << static_cast<int>(src->causality) << ", "
                                        << qualified << ", "
                                        << value_str << "\n";
                    }
                    else if (dynamic_cast<analysis::SspModelNode *>(child))
                    {
                        // Model children (connectors) already have model.var names.
                        // Pass prefix unchanged so the qualified name becomes
                        // <system_prefix>.<model_name>.<var_name>.
                        write_connectors(child, prefix);
                    }
                    else if (dynamic_cast<analysis::SspSystemNode *>(child))
                    {
                        std::string child_prefix =
                            prefix.empty() ? child->name : prefix + "." + child->name;
                        write_connectors(child, child_prefix);
                    }
                }
            };
            write_connectors(system_tree, "");
        }

        LOG_INFO(log, "[{func}] - Creating simulation models", __func__);
        auto sim_graph_builder = graph::GraphBuilder(config->record_inputs);

        SimulationPipelineResult result;
        result.models = sim_graph_builder.build(&analysis_graph_data);

        {
            std::vector<utils::graph::Node *> model_nodes;
            for (auto &[name, model] : result.models)
                model_nodes.push_back(model.get());
            std::ofstream(config->working_dir / "model_graph.dot") << utils::graph::Node::to_dot(model_nodes);
        }

        LOG_INFO(log, "[{func}] - Pipeline complete, {} models built", __func__, result.models.size());
        return result;
    }

} // namespace ssp4sim::pre