#include "analysis/components/analysis_model.hpp"

#include "FMI2_modelDescription_Ext.hpp"
#include "FMI2_Enums_Ext.hpp"

#include "fmu_info.hpp"

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
                ssp4cpp::utils::log::make_logger("ssp4sim.analysis.AnalysisModel");
            return logger;
        }
    }

    AnalysisModel::AnalysisModel(std::string name_, std::string source_path, std::unique_ptr<AnalysisParameterBindings> parameter_bindings_)
    {
        name = name_;
        type = ComponentType::Model;
        parameter_bindings = std::move(parameter_bindings_);

        auto fmu_tmp = std::make_unique<ssp4cpp::Fmu>(source_path);
        fmu = std::make_unique<handler::FmuInfo>(name, std::move(fmu_tmp));

        if (fmu->model_description->CoSimulation)
        {
            auto &co_sim = *fmu->model_description->CoSimulation;
            this->canInterpolateInputs = co_sim.canInterpolateInputs.value_or(false);
            this->maxOutputDerivativeOrder = co_sim.maxOutputDerivativeOrder.value_or(0);
        }

        create_connectors();
        create_model_variables();
    }

    AnalysisModel::~AnalysisModel() = default;

    std::string AnalysisModel::to_string() const
    {
        std::ostringstream oss;
        oss << "Model {"
            << "\n  name: " << name
            << "\n  source: " << fmu->fmi_instance->path()
            << "\n  delay: " << delay
            // << "\n  connectors: " << connectors.size()
            // << "\n  model_variables: " << model_variables.size()
            << "\n}";
        return oss.str();
    }

    /// Create FMU-level connectors for a given component and FMU info.
    void AnalysisModel::create_connectors()
    {
        connectors.clear();

        auto md = fmu->model_description;

        auto variables = ext::fmi2::model_variables::get_variables(
            *md, {types::Causality::input, types::Causality::output, types::Causality::parameter});

        for (auto &var : variables)
        {
            auto value_reference = var.valueReference.value();
            auto type = ext::fmi2::model_variables::get_variable_type(var);

            auto c = std::make_unique<AnalysisConnector>(name, var.name, value_reference, type, var.causality.value());

            c->model = this; // make sure model is not moved after this or pointer will be dangling

            auto start_value = ext::fmi2::model_variables::get_variable_start_value_or_default(var);

            auto sv = std::make_unique<ext::ParameterValue>(var.name, type);
            sv->store_value(start_value);

            c->initial_value = std::move(sv);

            connectors.push_back(std::move(c));
        }
    }

    void AnalysisModel::create_model_variables()
    {
        LOG_TRACE_L1(log(), "[{func}] init", __func__);

        model_variables.clear();

        auto md = fmu->model_description;

        auto dependencies = ext::fmi2::dependency::get_dependencies_variables(
            md->ModelStructure.Outputs.value().Unknowns,
            md->ModelVariables,
            ext::fmi2::DependenciesKind::dependent);

        for (auto &variable : fmu->model_description->ModelVariables.ScalarVariable)
        {
            auto mv = std::make_unique<AnalysisModelVariable>(variable);
            LOG_TRACE_L1(log(), "[{func}] New ModelVariable: {variable}", __func__, mv->name);

            mv->model = this; // make sure model is not moved after this or pointer will be dangling

            // populate variable dependencies
            for (auto &dep_coup : dependencies)
            {
                if (variable.valueReference == std::get<0>(dep_coup)->valueReference)
                {
                    mv->dependencies.emplace_back(std::get<1>(dep_coup));
                }
            }
            model_variables.emplace_back(std::move(mv));
        }
    }

} // namespace ssp4sim::analysis