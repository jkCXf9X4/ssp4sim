#include "analysis/analysis_system.hpp"

// Include stub data class headers (full implementations in Commit 2)
#include "analysis/analysis_model.hpp"
#include "analysis/analysis_connector.hpp"
#include "analysis/analysis_connection.hpp"

#include "tarjan.hpp"
#include "utils/node.hpp"

#include <sstream>
#include <stdexcept>
#include <utility>

namespace ssp4sim::analysis
{

    AnalysisSystem::AnalysisSystem(std::string name_)
        : name(std::move(name_))
    {
    }

    AnalysisSystem::~AnalysisSystem() = default;

    AnalysisGraphView AnalysisSystem::build_analysis_graph() const
    {
        std::vector<utils::graph::Node *> nodes;
        return AnalysisGraphView::from_nodes(std::move(nodes));
    }

    std::vector<std::vector<utils::graph::Node *>> AnalysisSystem::detect_algebraic_loops() const
    {
        auto graph_view = build_analysis_graph();
        if (graph_view.nodes.empty())
        {
            return {};
        }
        return ssp4sim::utils::graph::strongly_connected_components(graph_view.nodes);
    }

    std::vector<AnalysisModel *> AnalysisSystem::get_all_models() const
    {
        std::vector<AnalysisModel *> out;
        for (auto &m : models)
            out.push_back(m.get());
        for (auto &sys : nested_systems)
        {
            auto nested = sys->get_all_models();
            out.insert(out.end(), nested.begin(), nested.end());
        }
        return out;
    }

    std::vector<AnalysisConnection *> AnalysisSystem::get_all_connections() const
    {
        std::vector<AnalysisConnection *> out;
        for (auto &c : connections)
            out.push_back(c.get());
        for (auto &sys : nested_systems)
        {
            auto nested = sys->get_all_connections();
            out.insert(out.end(), nested.begin(), nested.end());
        }
        return out;
    }

    AnalysisConnector *AnalysisSystem::get_connector(const std::string &system_path,
                                                      const std::string &connector_name) const
    {
        if (system_path.empty() || system_path == name)
        {
            for (auto &conn : connectors)
            {
                if (conn->name == connector_name)
                    return conn.get();
            }
            return nullptr;
        }

        auto dot_pos = system_path.find('.');
        std::string head = (dot_pos == std::string::npos) ? system_path : system_path.substr(0, dot_pos);
        std::string tail = (dot_pos == std::string::npos) ? "" : system_path.substr(dot_pos + 1);

        for (auto &sys : nested_systems)
        {
            if (sys->name == head)
                return sys->get_connector(tail, connector_name);
        }
        return nullptr;
    }

    AnalysisSystem *AnalysisSystem::get_nested_system(const std::string &path) const
    {
        if (path.empty() || path == name)
            return const_cast<AnalysisSystem *>(this);

        auto dot_pos = path.find('.');
        std::string head = (dot_pos == std::string::npos) ? path : path.substr(0, dot_pos);
        std::string tail = (dot_pos == std::string::npos) ? "" : path.substr(dot_pos + 1);

        for (auto &sys : nested_systems)
        {
            if (sys->name == head)
                return sys->get_nested_system(tail);
        }
        return nullptr;
    }

} // namespace ssp4sim::analysis