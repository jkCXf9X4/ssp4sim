#include "graph_builder.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <memory>
#include <string>
#include <unordered_map>
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

        /// Collect all SspModelNode instances from the tree into a flat list.
        /// Recursively visits children.
        void collect_model_nodes(SspSystemNode *node,
                                 std::vector<std::unique_ptr<SspModelNode>> &out,
                                 std::unordered_map<std::string, SspModelNode *> &name_map)
        {
            for (auto *child : node->children)
            {
                if (auto *model_node = dynamic_cast<SspModelNode *>(child))
                {
                    auto ptr = std::make_unique<SspModelNode>(model_node->source);
                    auto *raw = ptr.get();
                    raw->name = model_node->name;
                    name_map[model_node->name] = raw;
                    out.push_back(std::move(ptr));
                }
                else if (auto *sys_node = dynamic_cast<SspSystemNode *>(child))
                {
                    collect_model_nodes(sys_node, out, name_map);
                }
            }
        }

        /// Collect all SspConnectorNode instances from the tree into a flat list.
        void collect_connector_nodes(SspSystemNode *node,
                                     std::vector<std::unique_ptr<SspConnectorNode>> &out,
                                     std::unordered_map<std::string, SspConnectorNode *> &name_map,
                                     std::unordered_map<std::string, SspModelNode *> &model_name_map)
        {
            for (auto *child : node->children)
            {
                if (auto *model_node = dynamic_cast<SspModelNode *>(child))
                {
                    // Find the matching model node in the flat list by name
                    auto it = model_name_map.find(model_node->name);
                    SspModelNode *flat_model = (it != model_name_map.end()) ? it->second : nullptr;

                    for (auto *grandchild : model_node->children)
                    {
                        if (auto *conn_node = dynamic_cast<SspConnectorNode *>(grandchild))
                        {
                            auto ptr = std::make_unique<SspConnectorNode>(conn_node->source);
                            auto *raw = ptr.get();
                            raw->name = conn_node->name;

                            // Link connector to its model
                            if (flat_model)
                            {
                                flat_model->add_child(raw);
                            }

                            name_map[conn_node->name] = raw;
                            out.push_back(std::move(ptr));
                        }
                    }
                }
                else if (auto *sys_node = dynamic_cast<SspSystemNode *>(child))
                {
                    // System-level connectors (boundary connectors)
                    for (auto *grandchild : sys_node->children)
                    {
                        if (auto *conn_node = dynamic_cast<SspConnectorNode *>(grandchild))
                        {
                            auto ptr = std::make_unique<SspConnectorNode>(conn_node->source);
                            auto *raw = ptr.get();
                            raw->name = conn_node->name;
                            name_map[conn_node->name] = raw;
                            out.push_back(std::move(ptr));
                        }
                    }

                    collect_connector_nodes(sys_node, out, name_map, model_name_map);
                }
            }
        }

    } // anonymous namespace

    AnalysisGraphData SspGraphBuilder::build(const SspSystemNode *tree)
    {
        AnalysisGraphData data;

        LOG_TRACE_L1(log(), "[{func}] Building graph from tree", __func__);

        // --- Pass 1: collect model nodes ---
        std::unordered_map<std::string, SspModelNode *> model_name_map;
        collect_model_nodes(const_cast<SspSystemNode *>(tree),
                            data.model_nodes, model_name_map);

        LOG_DEBUG(log(), "[{func}] Collected {} model nodes", __func__, data.model_nodes.size());

        // --- Pass 2: collect connector nodes and link to models ---
        std::unordered_map<std::string, SspConnectorNode *> connector_name_map;
        collect_connector_nodes(const_cast<SspSystemNode *>(tree),
                                data.connector_nodes, connector_name_map,
                                model_name_map);

        LOG_DEBUG(log(), "[{func}] Collected {} connector nodes", __func__, data.connector_nodes.size());

        // --- Pass 3: process connections ---
        // Walk the tree's SspConnectionNode entries and build the
        // model→connector→connection→connector→model graph
        auto walk_connections = [&](SspNode<SspConnection> *conn_node, auto &&self_ref) -> void
        {
            // This is a simplified approach: find connectors by name from the flat maps
            auto *conn = conn_node->source;

            std::string source_conn_name = conn->source_model + "." + conn->source_connector;
            std::string target_conn_name = conn->target_model + "." + conn->target_connector;

            auto src_it = connector_name_map.find(source_conn_name);
            auto tgt_it = connector_name_map.find(target_conn_name);

            if (src_it == connector_name_map.end() || tgt_it == connector_name_map.end())
            {
                LOG_WARNING(log(), "[{func}] Could not resolve connection: {src} -> {tgt}",
                            __func__, source_conn_name, target_conn_name);
                return;
            }

            auto *src_connector = src_it->second;
            auto *tgt_connector = tgt_it->second;

            // Create the ResolvedConnection (owned by connection_sources)
            auto rc = std::make_unique<ResolvedConnection>();
            rc->delay = conn->delay;
            rc->name = conn->name;
            auto *rc_raw = rc.get();

            // Create the SspNode wrapper (source pointer references rc_raw)
            auto resolved = std::make_unique<SspNode<ResolvedConnection>>(rc_raw);
            resolved->name = conn->name;
            auto *resolved_raw = resolved.get();

            // Build the graph chain:
            // source connector -> connection -> target connector
            src_connector->add_child(resolved_raw);
            resolved_raw->add_child(tgt_connector);

            data.connection_sources.push_back(std::move(rc));
            data.connection_nodes.push_back(std::move(resolved));
        };

        // Walk all SspConnectionNode instances in the tree
        // Use a recursive visitor
        auto visit_tree = [&](SspNode<SspSystem> *sys_node, auto &&visit_ref) -> void
        {
            for (auto *child : sys_node->children)
            {
                if (auto *conn_node = dynamic_cast<SspConnectionNode *>(child))
                {
                    walk_connections(conn_node, walk_connections);
                }
                else if (auto *sub_sys = dynamic_cast<SspSystemNode *>(child))
                {
                    visit_ref(sub_sys, visit_ref);
                }
                else if (auto *model_node = dynamic_cast<SspModelNode *>(child))
                {
                    // Model nodes might contain connections at a deeper level
                    // but connections are at system level in the tree
                    for (auto *grandchild : model_node->children)
                    {
                        if (auto *conn_node = dynamic_cast<SspConnectionNode *>(grandchild))
                        {
                            walk_connections(conn_node, walk_connections);
                        }
                    }
                }
            }
        };

        visit_tree(const_cast<SspSystemNode *>(tree), visit_tree);

        LOG_DEBUG(log(), "[{func}] Created {} connection nodes", __func__, data.connection_nodes.size());

        LOG_TRACE_L1(log(), "[{func}] exit", __func__);
        return data;
    }

} // namespace ssp4sim::analysis