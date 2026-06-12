#include "analysis/analysis_graph_factory.hpp"

#include "analysis/components/analysis_connector.hpp"
#include "analysis/components/analysis_connection.hpp"
#include "analysis/components/analysis_model.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <unordered_map>
#include <set>
#include <unordered_set>

namespace ssp4sim::analysis
{
    namespace
    {
        ssp4cpp::utils::log::Logger *factory_log()
        {
            static ssp4cpp::utils::log::Logger *logger =
                ssp4cpp::utils::log::make_logger("ssp4sim.analysis.AnalysisGraphFactory");
            return logger;
        }

        /// Helper: try to resolve a model name to its path-prefixed form.
        std::string resolve_model_name(const std::string &name,
                                       const std::vector<AnalysisModel *> &all_models)
        {
            if (name.empty()) return "";
            for (auto *model : all_models)
            {
                if (model->name == name ||
                    (model->name.size() > name.size() &&
                     model->name.substr(model->name.size() - name.size() - 1) == "." + name))
                {
                    return model->name;
                }
            }
            return name;
        }

        /// Build a map from connector full name to model pointer.
        std::unordered_map<std::string, AnalysisModel *>
        build_connector_model_map(const std::vector<AnalysisModel *> &all_models)
        {
            std::unordered_map<std::string, AnalysisModel *> map;
            for (auto *model : all_models)
            {
                for (auto &conn : model->connectors)
                {
                    map[conn->name] = model;
                }
            }
            return map;
        }
    }

    AnalysisGraphFactory::AnalysisGraphFactory(const AnalysisSystem &system)
        : system_(system)
    {
    }

    AnalysisGraphData AnalysisGraphFactory::build_all()
    {
        AnalysisGraphData data;

        data.model_nodes = build_model_graph();
        data.connector_nodes = build_connector_graph();

        // Build pre-resolved connection list (no string matching in GraphBuilder)
        auto all_models = system_.get_all_models();
        auto all_connections = system_.get_all_connections();

        for (auto *conn : all_connections)
        {
            // Resolve model names to path-prefixed form
            std::string src_model = resolve_model_name(conn->source_model, all_models);
            std::string tgt_model = resolve_model_name(conn->target_model, all_models);

            // If resolution failed or is empty, skip unresolvable connections
            if (src_model.empty() || tgt_model.empty())
            {
                // For boundary-crossing connections, one side may be empty: skip direct entry
                if (conn->is_boundary_crossing &&
                    (src_model.empty() || tgt_model.empty()))
                {
                    // Handle boundary crossing by walking through system hierarchy
                    auto *nested = system_.get_nested_system(
                        src_model.empty() ? conn->target_model : conn->source_model);
                    if (!nested)
                        continue;

                    for (auto &inner : nested->connections)
                    {
                        if (!inner->is_boundary_crossing) continue;
                        if (inner->source_model != "") continue;

                        // inner is a boundary connection inside the system
                        std::string connector_to_match = src_model.empty()
                            ? conn->source_connector
                            : conn->target_connector;
                        if (inner->source_connector != connector_to_match &&
                            inner->target_connector != connector_to_match)
                            continue;

                        std::string inner_tgt = nested->name + "." + inner->target_model;
                        inner_tgt = resolve_model_name(inner_tgt, all_models);
                        if (inner_tgt.empty()) continue;

                        if (src_model.empty())
                        {
                            ResolvedConnectionEntry entry;
                            entry.source_model = inner_tgt;
                            entry.source_connector = inner->target_connector;
                            entry.target_model = tgt_model;
                            entry.target_connector = conn->target_connector;
                            entry.delay = std::max(conn->delay, inner->delay);
                            data.resolved_connections.push_back(entry);
                        }
                        else
                        {
                            ResolvedConnectionEntry entry;
                            entry.source_model = src_model;
                            entry.source_connector = conn->source_connector;
                            entry.target_model = inner_tgt;
                            entry.target_connector = inner->target_connector;
                            entry.delay = std::max(conn->delay, inner->delay);
                            data.resolved_connections.push_back(entry);
                        }
                    }
                }
                continue;
            }

            auto src_name = AnalysisConnector::get_connector_name(src_model, conn->source_connector);
            auto tgt_name = AnalysisConnector::get_connector_name(tgt_model, conn->target_connector);

            ResolvedConnectionEntry entry;
            entry.source_model = src_model;
            entry.source_connector = src_name;
            entry.target_model = tgt_model;
            entry.target_connector = tgt_name;
            entry.delay = conn->delay;
            data.resolved_connections.push_back(entry);
        }

        return data;
    }

    std::vector<std::unique_ptr<ModelNode>> AnalysisGraphFactory::build_model_graph()
    {
        LOG_TRACE_L1(factory_log(), "[{func}] Building model graph", __func__);

        auto all_models = system_.get_all_models();

        // Create model nodes
        std::unordered_map<std::string, std::unique_ptr<ModelNode>> model_node_map;
        for (auto *model : all_models)
        {
            auto node = std::make_unique<ModelNode>(model->name, true, model);
            model_node_map[model->name] = std::move(node);
        }

        // Derive edges from connections (same logic as GraphBuilder::derive_model_edges)
        std::set<std::pair<std::string, std::string>> model_pairs;
        auto all_connections = system_.get_all_connections();

        for (auto *conn : all_connections)
        {
            // Resolve model names
            // For boundary-crossing connections, we need to track through hierarchy
            if (conn->is_boundary_crossing)
            {
                // Walk through system hierarchy for boundary connections
                std::string non_empty_side = conn->source_model.empty()
                    ? conn->target_model : conn->source_model;
                bool src_is_nonempty = !conn->source_model.empty();

                auto *nested = system_.get_nested_system(non_empty_side);
                if (!nested) continue;

                for (auto &inner : nested->connections)
                {
                    if (!inner->is_boundary_crossing) continue;
                    if (inner->source_model != "") continue;

                    std::string inner_tgt = nested->name + "." + inner->target_model;
                    std::string inner_tgt_resolved = resolve_model_name(inner_tgt, all_models);
                    if (inner_tgt_resolved.empty()) continue;

                    std::string resolved_non_empty = resolve_model_name(non_empty_side, all_models);
                    if (resolved_non_empty.empty()) continue;

                    if (src_is_nonempty)
                    {
                        model_pairs.insert({resolved_non_empty, inner_tgt_resolved});
                    }
                    else
                    {
                        model_pairs.insert({inner_tgt_resolved, resolved_non_empty});
                    }
                }
                continue;
            }

            std::string src_model = resolve_model_name(conn->source_model, all_models);
            std::string tgt_model = resolve_model_name(conn->target_model, all_models);

            // Resolve through system hierarchy if needed
            if (!src_model.empty() && !tgt_model.empty())
            {
                model_pairs.insert({src_model, tgt_model});
            }
            else if (!src_model.empty() && tgt_model.empty() && !conn->target_model.empty())
            {
                auto *nested = system_.get_nested_system(conn->target_model);
                if (nested)
                {
                    for (auto &inner : nested->connections)
                    {
                        if (!inner->is_boundary_crossing) continue;
                        if (inner->source_model != "") continue;
                        if (inner->source_connector != conn->target_connector) continue;
                        std::string inner_tgt = nested->name + "." + inner->target_model;
                        inner_tgt = resolve_model_name(inner_tgt, all_models);
                        if (inner_tgt.empty()) continue;
                        model_pairs.insert({src_model, inner_tgt});
                    }
                }
            }
            else if (!tgt_model.empty() && src_model.empty() && !conn->source_model.empty())
            {
                auto *nested = system_.get_nested_system(conn->source_model);
                if (nested)
                {
                    for (auto &inner : nested->connections)
                    {
                        if (!inner->is_boundary_crossing) continue;
                        if (inner->source_model != "") continue;
                        if (inner->source_connector != conn->source_connector) continue;
                        std::string inner_tgt = nested->name + "." + inner->target_model;
                        inner_tgt = resolve_model_name(inner_tgt, all_models);
                        if (inner_tgt.empty()) continue;
                        model_pairs.insert({inner_tgt, tgt_model});
                    }
                    for (auto &inner : nested->connections)
                    {
                        if (!inner->is_boundary_crossing) continue;
                        if (inner->target_model != "") continue;
                        if (inner->target_connector != conn->source_connector) continue;
                        std::string inner_src = nested->name + "." + inner->source_model;
                        inner_src = resolve_model_name(inner_src, all_models);
                        if (inner_src.empty()) continue;
                        model_pairs.insert({inner_src, tgt_model});
                    }
                }
            }
        }

        // Add edges: source -> target (source must execute before target)
        for (auto &[source_name, target_name] : model_pairs)
        {
            auto src_it = model_node_map.find(source_name);
            auto tgt_it = model_node_map.find(target_name);
            if (src_it != model_node_map.end() && tgt_it != model_node_map.end())
            {
                src_it->second->add_child(tgt_it->second.get());
            }
        }

        // Collect nodes into vector
        std::vector<std::unique_ptr<ModelNode>> result;
        for (auto &[name, node] : model_node_map)
        {
            result.push_back(std::move(node));
        }

        return result;
    }

    std::vector<std::unique_ptr<ConnectorNode>> AnalysisGraphFactory::build_connector_graph()
    {
        LOG_TRACE_L1(factory_log(), "[{func}] Building connector graph", __func__);

        auto all_models = system_.get_all_models();
        auto all_connections = system_.get_all_connections();

        // Build connector-to-model map for setting model pointers
        auto conn_model_map = build_connector_model_map(all_models);

        // Create nodes for all model connectors with model pointer set
        std::unordered_map<std::string, std::unique_ptr<ConnectorNode>> node_map;

        for (auto *model : all_models)
        {
            for (auto &conn : model->connectors)
            {
                auto node = std::make_unique<ConnectorNode>(
                    conn->name, true, conn.get(), model);
                node_map[conn->name] = std::move(node);
            }
        }

        // Wire connections: add edge from target connector to source connector
        // (dependency direction for algebraic loop detection)
        for (auto *conn : all_connections)
        {
            std::string src_model = resolve_model_name(conn->source_model, all_models);
            std::string tgt_model = resolve_model_name(conn->target_model, all_models);

            // For boundary-crossing connections, resolve through hierarchy
            if (conn->is_boundary_crossing ||
                src_model.empty() || tgt_model.empty())
            {
                // Skip unresolvable connections in the algebraic graph
                // (the resolved connections list in build_all handles them)
                continue;
            }

            auto src_name = AnalysisConnector::get_connector_name(src_model, conn->source_connector);
            auto tgt_name = AnalysisConnector::get_connector_name(tgt_model, conn->target_connector);

            auto src_it = node_map.find(src_name);
            auto tgt_it = node_map.find(tgt_name);

            if (src_it != node_map.end() && tgt_it != node_map.end())
            {
                if (!conn->is_boundary_crossing)
                {
                    tgt_it->second->add_child(src_it->second.get());
                }
            }
        }

        // Collect nodes into vector
        std::vector<std::unique_ptr<ConnectorNode>> result;
        for (auto &[name, node] : node_map)
        {
            result.push_back(std::move(node));
        }

        return result;
    }

    std::vector<std::vector<utils::graph::Node *>> AnalysisGraphFactory::find_algebraic_loops()
    {
        auto connector_nodes = build_connector_graph();
        if (connector_nodes.empty())
        {
            return {};
        }

        // Non-owning pointer vector for Tarjan SCC
        std::vector<utils::graph::Node *> raw_nodes;
        raw_nodes.reserve(connector_nodes.size());
        for (auto &node : connector_nodes)
        {
            raw_nodes.push_back(node.get());
        }

        auto sccs = utils::graph::strongly_connected_components(raw_nodes);

        // Filter multi-node SCCs and track which nodes are in loops
        std::vector<std::vector<utils::graph::Node *>> loops;
        std::unordered_set<utils::graph::Node *> loop_nodes;
        for (auto &scc : sccs)
        {
            if (scc.size() > 1)
            {
                for (auto *n : scc)
                {
                    loop_nodes.insert(n);
                }
                loops.push_back(std::move(scc));
            }
        }

        // Release loop nodes from unique_ptr ownership (caller becomes owner)
        for (auto &node : connector_nodes)
        {
            if (loop_nodes.count(node.get()))
            {
                node.release();
            }
        }

        return loops;
    }

} // namespace ssp4sim::analysis