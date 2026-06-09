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

    const AnalysisConnector *AnalysisSystem::find_connector(
        const std::string &model_name,
        const std::string &connector_full_name) const
    {
        for (auto *m : get_all_models())
        {
            if (m->name != model_name)
                continue;
            for (auto &c : m->connectors)
            {
                if (c->name == connector_full_name)
                    return c.get();
            }
        }
        return nullptr;
    }

    std::string AnalysisSystem::to_string() const
    {
        std::ostringstream oss;
        oss << "System {\n"
            << "  name: " << name << "\n"
            << "  models: " << models.size() << "\n"
            << "  boundary_connectors: " << connectors.size() << "\n"
            << "  connections: " << connections.size() << "\n"
            << "  nested_systems: " << nested_systems.size() << "\n"
            << "}";
        return oss.str();
    }

    std::string AnalysisSystem::tree_string() const
    {
        return tree_string_impl("");
    }

    std::string AnalysisSystem::tree_string_impl(const std::string &indent) const
    {
        std::ostringstream oss;
        oss << indent << "System: " << name << "\n";

        std::string child_indent = indent + "  ";

        oss << child_indent << "Models (" << models.size() << "):\n";
        for (const auto &m : models)
        {
            oss << child_indent << "  " << m->name << "\n";
            if (m->model_variables.size() > 0 || m->connectors.size() > 0)
            {
                oss << child_indent << "    connectors: " << m->connectors.size() << "\n";
                for (const auto &c : m->connectors)
                {
                    oss << child_indent << "      " << c->name << "\n";
                }
                oss << child_indent << "    model_variables: " << m->model_variables.size() << "\n";
            }
        }

        oss << child_indent << "Boundary Connectors (" << connectors.size() << "):\n";
        for (const auto &c : connectors)
        {
            oss << child_indent << "  " << c->name << "\n";
        }

        oss << child_indent << "Connections (" << connections.size() << "):\n";
        for (const auto &c : connections)
        {
            oss << child_indent << "  " << c->source_model << "." << c->source_connector
                << " -> " << c->target_model << "." << c->target_connector << "\n";
        }

        oss << child_indent << "Nested Systems (" << nested_systems.size() << "):\n";
        for (const auto &ns : nested_systems)
        {
            oss << ns->tree_string_impl(child_indent + "  ");
        }

        return oss.str();
    }

} // namespace ssp4sim::analysis