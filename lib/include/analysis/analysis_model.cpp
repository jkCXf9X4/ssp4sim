#include "analysis/analysis_model.hpp"

#include "handler/fmu_handler.hpp"

#include "ssp4cpp/utils/log.hpp"

#include "FMI2_modelDescription_Ext.hpp"
#include "FMI2_Enums_Ext.hpp"

#include <sstream>
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

    // TODO: The feedthrough can pass thru many internal variables before entering out on the other side
    // It can also ba a 1 to many mapping between in and out
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
            auto dependencies = ext::fmi2::dependency::get_dependencies_variables(
                md->ModelStructure.Outputs.value().Unknowns,
                md->ModelVariables,
                ext::fmi2::DependenciesKind::dependent);

            for (const auto &[output_var, dep_var, kind] : dependencies)
            {
                (void)kind;
                auto dep_connector_name = name + "." + dep_var->name;
                if (input_connector_names.count(dep_connector_name))
                {
                    auto output_connector_name = name + "." + output_var->name;
                    for (auto &conn : connectors)
                    {
                        if (conn->name == output_connector_name)
                        {
                            conn->is_feedthrough = true;
                            break;
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