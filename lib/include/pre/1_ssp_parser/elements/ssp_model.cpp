#include "ssp_model.hpp"

#include "../schema_extensions/FMI2_modelDescription_Ext.hpp"
#include "../schema_extensions/FMI2_Enums_Ext.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace ssp4sim::analysis
{
    namespace
    {
        ssp4cpp::utils::log::Logger *log()
        {
            static ssp4cpp::utils::log::Logger *logger =
                ssp4cpp::utils::log::make_logger("ssp4sim.analysis.SspModel");
            return logger;
        }
    }

SspModel::SspModel(std::string name_,
                                 std::string source_path,
                                 std::map<std::string, ext::ParameterValue> parameter_bindings_)
    : parameter_bindings(parameter_bindings_)
{
    name = name_;
    type = SspItemType::Model;

    this->fmu = std::make_shared<ssp4cpp::Fmu>(source_path);

    if (fmu->md->CoSimulation)
    {
        auto &co_sim = *fmu->md->CoSimulation;
        this->canInterpolateInputs = co_sim.canInterpolateInputs.value_or(false);
        this->maxOutputDerivativeOrder = co_sim.maxOutputDerivativeOrder.value_or(0);
    }

    create_connectors();
}

    std::string SspModel::to_string() const
    {
        std::ostringstream oss;
        oss << "Model {"
            << "\n  name: " << name
            << "\n  delay: " << delay
            << "\n}";
        return oss.str();
    }

    /// Create FMU-level connectors for a given component and FMU info.
    void SspModel::create_connectors()
    {
        connectors.clear();

        auto dependencies = ext::fmi2::dependency::get_dependencies_variables(
            fmu->md->ModelStructure.Outputs.value().Unknowns,
            fmu->md->ModelVariables,
            ext::fmi2::DependenciesKind::dependent);

        auto variables = ext::fmi2::model_variables::get_variables(
            *fmu->md, {types::Causality::input, types::Causality::output, types::Causality::parameter});

        for (auto &var : variables)
        {
            auto value_reference = var.valueReference.value();
            auto type = ext::fmi2::model_variables::get_variable_type(var);

            auto connector_name = name + "." + var.name;
            auto c = SspConnector(connector_name, value_reference, type, var.causality.value());

            auto start_value = ext::fmi2::model_variables::get_variable_start_value(var);
            if (start_value != nullptr)
            {
                c.initial_value.store_value(start_value);
            }

            for (auto &dep_coup : dependencies)
            {
                if (var.valueReference == dep_coup.target->valueReference)
                {
                    c.dependencies.emplace_back(dep_coup.dependency);
                }
            }

            connectors.push_back(std::move(c));
        }


        model_variables.clear();


        for (auto &variable : fmu->md->ModelVariables.ScalarVariable)
        {
            auto mv = SspModelVariable(variable);
            LOG_TRACE_L1(log(), "[{func}] New ModelVariable: {variable}", __func__, mv.name);

            // populate variable dependencies
            for (auto &dep_coup : dependencies)
            {
                if (variable.valueReference == dep_coup.target->valueReference)
                {
                    mv.dependencies.emplace_back(dep_coup.dependency);
                }
            }
            model_variables.emplace_back(std::move(mv));
        }
    }

} // namespace ssp4sim::analysis