#include "analysis/components/analysis_system.hpp"

// Include stub data class headers (full implementations in Commit 2)
#include "analysis/components/analysis_model.hpp"
#include "analysis/components/analysis_connector.hpp"
#include "analysis/components/analysis_connection.hpp"

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

    AnalysisSystem::AnalysisSystem(const ssp4cpp::ssp1::ssd::TSystem &sys, handler::FmuHandler *fmu_handler)
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

            if (!fmu_handler->fmu_info_map.contains(component_name))
            {
                LOG_ERROR(log(), "[{func}] FMU not found: {name}", __func__, component_name);
                throw std::runtime_error("FMU not found: " + component_name);
            }

            auto fmu_info = fmu_handler->fmu_info_map[component_name].get();

            auto model = std::make_unique<AnalysisModel>(fmu_info);

            models.push_back(std::move(model));
        }

        for (auto &sub_sys : elements.Systems)
        {
            auto nested = std::make_unique<AnalysisSystem>(sub_sys, fmu_handler);
            nested_systems.push_back(std::move(nested));
        }

        // // Process system-level (boundary) connectors

        for (auto & connector : sys.Connectors)
        {
            if (conn)
        }
        // builder.process_boundary_connectors(*analysis_sys, sys);

        // // Process connections at this system level
        // builder.process_connections(*analysis_sys, sys);
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

} // namespace ssp4sim::analysis