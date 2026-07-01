#include "analysis/components/analysis_system.hpp"

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
                ssp4cpp::utils::log::make_logger("ssp4sim.analysis.SspSystem");
            return logger;
        }
    }

    SspSystem::SspSystem(const ssp4cpp::ssp1::ssd::TSystem &sys, ssp4cpp::Ssp *ssp)
    {
        type = SspItemType::System;
        name = sys.name.value_or("unnamed");

        if (!sys.Elements.has_value())
        {
            return;
        }

        if (sys.ParameterBindings.has_value())
        {
            bindings = ext::ssp1::ssv::get_start_value_mappings(
                sys.ParameterBindings->ParameterBindings, ssp);
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

            auto comp_bindings = AnalysisParameterBindings(component.ParameterBindings, ssp);

            auto model = SspModel(component_name, ssp->dir / component.source, comp_bindings);

            models.push_back(model);
        }

        for (auto &sub_sys : elements.Systems)
        {
            auto nested = SspSystem(sub_sys, ssp);
            nested_systems.push_back(nested);
        }

        // Process system-level (boundary) connectors
        if (sys.Connectors.has_value())
        {
            for (auto &connector : sys.Connectors.value().Connectors)
            {
                auto analysis_conn = SspConnector(
                    connector.name,
                    0, // no value reference for system-level connectors
                    types::DataType::unknown,
                    connector.kind);

                connectors.push_back(analysis_conn);
            }
        }

        // Process connections at this system level
        if (sys.Connections.has_value())
        {
            for (auto &conn : sys.Connections.value().Connections)
            {
                std::string src_model_str = conn.startElement.value_or("");
                std::string tgt_model_str = conn.endElement.value_or("");

                std::string src_con = conn.startConnector;
                std::string tgt_con = conn.endConnector;

                auto analysis_conn = std::make_unique<Connection>(src_model_str, src_con, tgt_model_str, tgt_con);

                // add optional delay....

                connections.push_back(analysis_conn);
            }
        }
    }

    SspSystem::~SspSystem() = default;

    std::string SspSystem::to_string() const
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

} // namespace ssp4sim::analysis