#include "analysis/components/analysis_system.hpp"

// Include stub data class headers (full implementations in Commit 2)
#include "analysis/components/analysis_model.hpp"
#include "analysis/components/analysis_connector.hpp"
#include "analysis/components/analysis_connection.hpp"

#include "analysis/analysis_graph_factory.hpp"

#include "tarjan.hpp"
#include "utils/node.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <sstream>
#include <stdexcept>
#include <utility>

namespace ssp4sim::analysis
{
    namespace
    {
        ssp4cpp::utils::log::Logger *log()
        {
            static ssp4cpp::utils::log::Logger *logger =
                ssp4cpp::utils::log::make_logger("ssp4sim.analysis.AnalysisSystem");
            return logger;
        }
    }

    AnalysisSystem::AnalysisSystem(const std::string &name_)
        : name(name_)
    {
    }

    AnalysisSystem::AnalysisSystem(const ssp4cpp::ssp1::ssd::TSystem &sys, handler::FmuHandler *fmu_handler, const std::string &path_prefix)
        : name(sys.name.value_or("unnamed"))
    {
        if (!sys.Elements.has_value())
        {
            return;
        }

        auto &elements = sys.Elements.value();

        for (auto &component : elements.Components)
        {
            if (!component.name.has_value())
            {
                LOG_ERROR(log(), "[{func}] Component does not specify name attribute", __func__);
                throw std::runtime_error("Component without name");
            }

            auto component_name = component.name.value();
            auto fmu_lookup_name = path_prefix.empty() ? component_name : path_prefix + "." + component_name;

            if (!fmu_handler->fmu_info_map.contains(fmu_lookup_name))
            {
                LOG_ERROR(log(), "[{func}] FMU not found: {name}", __func__, fmu_lookup_name);
                throw std::runtime_error("FMU not found: " + fmu_lookup_name);
            }

            auto fmu_info = fmu_handler->fmu_info_map[fmu_lookup_name].get();

            auto model = std::make_unique<AnalysisModel>(fmu_info, fmu_lookup_name);

            models.push_back(std::move(model));
        }

        for (auto &sub_sys : elements.Systems)
        {
            auto sub_sys_name = sub_sys.name.value_or("unnamed");
            auto sub_prefix = path_prefix.empty() ? sub_sys_name : path_prefix + "." + sub_sys_name;
            auto nested = std::make_unique<AnalysisSystem>(sub_sys, fmu_handler, sub_prefix);
            nested_systems.push_back(std::move(nested));
        }

        // Process system-level (boundary) connectors
        if (sys.Connectors.has_value())
        {
            for (auto &connector : sys.Connectors.value().Connectors)
            {
                auto analysis_conn = std::make_unique<AnalysisConnector>(
                    sys.name.value_or("unnamed"),
                    connector.name,
                    0, // no value reference for system-level connectors
                    types::DataType::unknown);
                analysis_conn->is_boundary = true;
                analysis_conn->causality = connector.kind;
                connectors.push_back(std::move(analysis_conn));
            }
        }

        // Process connections at this system level
        if (sys.Connections.has_value())
        {
            for (auto &conn : sys.Connections.value().Connections)
            {
                bool is_boundary = !conn.startElement.has_value() || !conn.endElement.has_value();
                std::string src_model = conn.startElement.value_or("");
                std::string src_con  = conn.startConnector;
                std::string tgt_model = conn.endElement.value_or("");
                std::string tgt_con  = conn.endConnector;
                auto analysis_conn = std::make_unique<AnalysisConnection>(
                    src_model, src_con, tgt_model, tgt_con, 0, is_boundary);
                connections.push_back(std::move(analysis_conn));
            }
        }
    }

    AnalysisSystem::~AnalysisSystem() = default;

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

    std::vector<AnalysisModel *> AnalysisSystem::get_all_models() const
    {
        std::vector<AnalysisModel *> out;
        collect_models(out);
        return out;
    }

    std::vector<AnalysisConnection *> AnalysisSystem::get_all_connections() const
    {
        std::vector<AnalysisConnection *> out;
        collect_connections(out);
        return out;
    }

    void AnalysisSystem::collect_models(std::vector<AnalysisModel *> &out) const
    {
        for (auto &m : models)
            out.push_back(m.get());
        for (auto &sys : nested_systems)
            sys->collect_models(out);
    }

    void AnalysisSystem::collect_connections(std::vector<AnalysisConnection *> &out) const
    {
        for (auto &c : connections)
            out.push_back(c.get());
        for (auto &sys : nested_systems)
            sys->collect_connections(out);
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

    std::vector<std::vector<utils::graph::Node *>> AnalysisSystem::detect_algebraic_loops() const
    {
        AnalysisGraphFactory factory(*this);
        return factory.find_algebraic_loops();
    }

    AnalysisGraphView AnalysisSystem::build_analysis_graph() const
    {
        AnalysisGraphFactory factory(*this);
        auto nodes = factory.build_transient_graph();
        return AnalysisGraphView::from_nodes(std::move(nodes));
    }

} // namespace ssp4sim::analysis