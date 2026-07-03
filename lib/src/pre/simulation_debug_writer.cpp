#include "simulation_debug_writer.hpp"

#include "pre/1_ssp_parser/schema_extensions/FMI2_Enums_Ext.hpp"
#include "pre/ssp_graph_data.hpp"
#include "shared_config.hpp"

#include "execution/invocable.hpp"
#include "utils/primitives/node.hpp"

#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ssp4sim::pre
{

    SimulationDebugWriter::SimulationDebugWriter(SharedConfig *config)
        : config_(config)
    {
    }

    void SimulationDebugWriter::write_analysis_debug(
        analysis::SspSystemNode *system_tree,
        const analysis::AnalysisGraphData &graph_data)
    {
        std::ofstream(config_->working_dir / "tree_output.txt") << system_tree->get_tree();

        std::vector<utils::graph::Node *> all_nodes;
        for (auto &n : graph_data.model_nodes)
            all_nodes.push_back(n.get());
        for (auto &n : graph_data.connector_nodes)
            all_nodes.push_back(n.get());
        for (auto &n : graph_data.connection_nodes)
            all_nodes.push_back(n.get());
        std::ofstream(config_->working_dir / "graph_output.dot") << utils::graph::Node::to_dot(all_nodes);
    }

    void SimulationDebugWriter::write_start_values(utils::graph::Node *system_tree)
    {
        auto start_value_log = std::ofstream(config_->start_value_log_file, std::ios::out);
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

    void SimulationDebugWriter::write_model_graph(
        const std::map<std::string, std::unique_ptr<graph::Invocable>> &models)
    {
        std::vector<utils::graph::Node *> model_nodes;
        for (auto &[name, model] : models)
            model_nodes.push_back(model.get());
        std::ofstream(config_->working_dir / "model_graph.dot") << utils::graph::Node::to_dot(model_nodes);
    }

} // namespace ssp4sim::pre