#include "analysis/analysis_graph_factory.hpp"

#include "analysis/components/analysis_connector.hpp"
#include "analysis/components/analysis_connection.hpp"
#include "analysis/components/analysis_model.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <unordered_map>

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
    }

    AnalysisGraphFactory::AnalysisGraphFactory(const AnalysisSystem &system)
        : system_(system)
    {
    }

    std::vector<utils::graph::Node *> AnalysisGraphFactory::build_transient_graph()
    {
        LOG_TRACE_L1(factory_log(), "[{func}] Building transient graph", __func__);

        // Collect all models and their connectors
        auto all_models = system_.get_all_models();

        // Map from connector name to AnalysisNode
        std::unordered_map<std::string, std::unique_ptr<AnalysisNode>> node_map;

        // Create nodes for all model connectors
        for (auto *model : all_models)
        {
            for (auto &conn : model->connectors)
            {
                auto node = std::make_unique<AnalysisNode>(
                    conn->name, true, conn.get());
                node_map[conn->name] = std::move(node);
            }
        }

        // Wire connections: for each connection, add edge from source connector to target connector
        auto all_connections = system_.get_all_connections();
        for (auto *conn : all_connections)
        {
            auto src_name = AnalysisConnector::get_connector_name(conn->source_model, conn->source_connector);
            auto tgt_name = AnalysisConnector::get_connector_name(conn->target_model, conn->target_connector);

            auto src_it = node_map.find(src_name);
            auto tgt_it = node_map.find(tgt_name);

            if (src_it != node_map.end() && tgt_it != node_map.end())
            {
                // Connection: source output → target input
                // In the graph, add edge from target to source (dependency direction)
                if (!conn->is_boundary_crossing)
                {
                    tgt_it->second->add_child(src_it->second.get());
                }
            }
            else
            {
                LOG_DEBUG(factory_log(), "[{func}] Skipping connection {src} -> {tgt}: connector nodes not found",
                          __func__, src_name, tgt_name);
            }
        }

        // Collect all nodes into a vector, releasing ownership to the caller
        std::vector<utils::graph::Node *> result;
        for (auto &[name, node] : node_map)
        {
            result.push_back(node.release());
        }

        return result;
    }

    std::vector<std::vector<utils::graph::Node *>> AnalysisGraphFactory::find_algebraic_loops()
    {
        auto nodes = build_transient_graph();
        if (nodes.empty())
        {
            return {};
        }

        // Run Tarjan SCC
        auto sccs = utils::graph::strongly_connected_components(nodes);

        // Filter out single-node components (those are not loops)
        std::vector<std::vector<utils::graph::Node *>> loops;
        for (auto &scc : sccs)
        {
            if (scc.size() > 1)
            {
                loops.push_back(std::move(scc));
            }
        }

        // Clean up transient nodes (those not in loops still need cleanup)
        // Nodes in loops are moved out, so we delete remaining
        for (auto *n : nodes)
        {
            // Check if node is still owned by us (not moved into loops)
            bool found = false;
            for (auto &loop : loops)
            {
                for (auto *ln : loop)
                {
                    if (ln == n)
                    {
                        found = true;
                        break;
                    }
                }
                if (found) break;
            }
            if (!found)
            {
                delete n;
            }
        }

        return loops;
    }

} // namespace ssp4sim::analysis