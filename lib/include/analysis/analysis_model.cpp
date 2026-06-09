#include "analysis/analysis_model.hpp"

#include "handler/fmu_handler.hpp"

#include "ssp4cpp/utils/log.hpp"

#include "FMI2_modelDescription_Ext.hpp"
#include "FMI2_Enums_Ext.hpp"

#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace ssp4sim::analysis
{
namespace
{
    ssp4cpp::utils::log::Logger* model_log()
    {
        static ssp4cpp::utils::log::Logger* logger =
            ssp4cpp::utils::log::make_logger("ssp4sim.analysis.AnalysisModel");
        return logger;
    }
}

    AnalysisModel::AnalysisModel(std::string name_,
                                  std::string source_file_,
                                  handler::FmuInfo *fmu_)
        : name(std::move(name_)),
          source_file(std::move(source_file_)),
          fmu(fmu_)
    {
    }

    AnalysisModel::~AnalysisModel() = default;

    std::string AnalysisModel::to_string() const
    {
        std::ostringstream oss;
        oss << "Model {"
            << "\n  name: " << name
            << "\n  type: " << type
            << "\n  source: " << source_file
            << "\n  delay: " << delay
            << "\n  connectors: " << connectors.size()
            << "\n  model_variables: " << model_variables.size()
            << "\n}";
        return oss.str();
    }

    /// Compute feedthrough marking on this model's connectors from FMU ModelStructure.
    ///
    /// Uses FMI2 dependency data to trace direct and transitive paths from each
    /// output connector back to input connectors.  For outputs whose direct
    /// dependencies are internal (non-input) model variables, a BFS walks the
    /// dependency chain through intermediate outputs until an input is found or
    /// all paths are exhausted.  Supports 1-to-many input→output mappings.
    void AnalysisModel::compute_feedthrough(handler::FmuInfo *fmu_info)
    {
        auto *md = fmu_info->model_description;
        if (!md->ModelStructure.Outputs.has_value())
            return;

        // Build set of input connector names for quick lookup
        std::unordered_set<std::string> input_connector_names;
        for (const auto &conn : connectors)
        {
            if (conn->causality == types::Causality::input)
                input_connector_names.insert(conn->name);
        }

        if (input_connector_names.empty())
            return;

        try
        {
            // Get all dependencies (output → dep_var) filtered by dependent kind
            auto dependencies = ext::fmi2::dependency::get_dependencies_variables(
                md->ModelStructure.Outputs.value().Unknowns,
                md->ModelVariables,
                ext::fmi2::DependenciesKind::dependent);

            // Build adjacency: output variable pointer → list of (dep_var, kind)
            std::unordered_map<const ext::fmi2::fmi2ScalarVariable *,
                               std::vector<std::pair<const ext::fmi2::fmi2ScalarVariable *,
                                                     ext::fmi2::DependenciesKind>>>
                dep_map;

            // Also collect the set of known output variables (those that appear
            // as an Unknown in the ModelStructure) for transitive BFS.
            std::unordered_set<const ext::fmi2::fmi2ScalarVariable *> output_variables;

            for (const auto &[output_var, dep_var, kind] : dependencies)
            {
                dep_map[output_var].emplace_back(dep_var, kind);
                output_variables.insert(output_var);
            }

            for (const auto &[output_var, dep_list] : dep_map)
            {
                (void)dep_list;
                auto output_connector_name = name + "." + output_var->name;

                // Check each direct dependency of this output
                for (const auto &[dep_var, kind] : dep_map.at(output_var))
                {
                    (void)kind;
                    auto dep_connector_name = name + "." + dep_var->name;

                    if (input_connector_names.count(dep_connector_name))
                    {
                        // Direct feedthrough: output depends directly on an input
                        for (auto &conn : connectors)
                        {
                            if (conn->name == output_connector_name)
                            {
                                conn->is_feedthrough = true;
                                break;
                            }
                        }
                    }
                    else if (output_variables.count(dep_var))
                    {
                        // Transitive: dep_var is itself an output of some Unknown.
                        // BFS through the dependency chain to find an input.
                        std::unordered_set<const ext::fmi2::fmi2ScalarVariable *> visited;
                        std::vector<const ext::fmi2::fmi2ScalarVariable *> stack;
                        stack.push_back(dep_var);

                        while (!stack.empty())
                        {
                            auto *cur = stack.back();
                            stack.pop_back();

                            if (!visited.insert(cur).second)
                                continue;

                            // Look up cur's own dependencies (it appears as an output)
                            auto it = dep_map.find(cur);
                            if (it == dep_map.end())
                                continue;

                            for (const auto &[next_dep, next_kind] : it->second)
                            {
                                (void)next_kind;
                                auto next_name = name + "." + next_dep->name;

                                if (input_connector_names.count(next_name))
                                {
                                    // Found a transitive path to an input
                                    for (auto &conn : connectors)
                                    {
                                        if (conn->name == output_connector_name)
                                        {
                                            conn->is_feedthrough = true;
                                            break;
                                        }
                                    }
                                    // Clear the stack/visited to break out of BFS
                                    // since we already marked this output.
                                    stack.clear();
                                    visited.clear();
                                    break;
                                }

                                // Continue BFS if next_dep is also an output
                                if (output_variables.count(next_dep))
                                {
                                    stack.push_back(next_dep);
                                }
                            }
                        }
                    }
                }
            }
        }
        catch (const std::exception &e)
        {
            LOG_WARNING(model_log(), "[{func}] Skipping feedthrough for FMU {fmu}: {reason}",
                        __func__, name, e.what());
        }
    }

} // namespace ssp4sim::analysis