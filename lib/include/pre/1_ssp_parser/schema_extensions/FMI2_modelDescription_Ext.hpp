
#pragma once

#include "ssp4cpp/utils/log.hpp"

#include "ssp4cpp/schema/fmi2/FMI2_Enums.hpp"
#include "ssp4cpp/schema/fmi2/FMI2_modelDescription.hpp"

#include "ssp4sim_definitions.hpp"

#include <initializer_list>
#include <vector>
#include <string>
#include <tuple>

namespace ssp4sim::ext::fmi2
{
    using namespace ssp4cpp::fmi2::md;

    namespace model_variables
    {
        /** @brief Retrieve a variable by index from the model variables list. */
        fmi2ScalarVariable *get_variable(ModelVariables &mv, int index);

        fmi2ScalarVariable *get_variable_by_name(ModelVariables &mv, std::string name);

        types::DataType get_variable_type(fmi2ScalarVariable &var);

        void *get_variable_start_value(fmi2ScalarVariable &var);

        void *get_variable_start_value_or_default(fmi2ScalarVariable &var);

        std::vector<fmi2ScalarVariable> get_variables(
            fmi2ModelDescription &md,
            std::initializer_list<types::Causality> causalities);
    }

    namespace dependency
    {

        struct VariableIndexDependency
        {
            // target depends on dependency
            int target;
            int dependency;
            DependenciesKind kind;
        };

        struct VariableDependency
        {
            // target depends on dependency
            fmi2ScalarVariable * target;
            fmi2ScalarVariable * dependency;
            DependenciesKind kind;
        };

        // Unknowns
        /** @brief Get variable dependencies for an Unknown by index. */
        std::vector<VariableIndexDependency> get_dependencies_index(Unknown &u);

        /** @brief Get variable dependencies for an Unknown filtered by kind. */
        std::vector<VariableIndexDependency> get_dependencies_index(Unknown &u, DependenciesKind kind);

        /** @brief Resolve variable dependencies for a single Unknown. */
        std::vector<VariableDependency> get_dependencies_variables(Unknown &u, ModelVariables &mv);

        /** @brief Resolve variable dependencies filtered by kind. */
        std::vector<VariableDependency> get_dependencies_variables(Unknown &u, ModelVariables &mv, DependenciesKind kind);

        /** @brief Resolve dependencies for multiple Unknowns filtered by kind. */
        std::vector<VariableDependency> get_dependencies_variables(std::vector<Unknown> &us, ModelVariables &mv, DependenciesKind kind);
    }

}
