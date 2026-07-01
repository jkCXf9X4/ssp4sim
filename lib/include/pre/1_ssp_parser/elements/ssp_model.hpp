#pragma once

#include "../schema_extensions/parameter_value.hpp"

#include "_ssp_item.hpp"

#include "ssp_connector.hpp"
#include "ssp_model_variable.hpp"

#include "ssp4cpp/fmu.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>



namespace ssp4sim::analysis
{

    class SspModel : public SspItem
    {
    public:
        std::vector<SspConnector> connectors;
        std::vector<SspModelVariable> model_variables;

        std::map<std::string, ext::ParameterValue> parameter_bindings;

        uint64_t delay = 0;
        std::shared_ptr<ssp4cpp::Fmu> fmu;

        bool canInterpolateInputs = false;
        int maxOutputDerivativeOrder = 0;

        SspModel(std::string name_,
                      std::string source_path,
                      std::map<std::string, ext::ParameterValue> parameter_bindings_);

        std::string to_string() const;

    private:
        void create_connectors();
        void create_model_variables();
    };

} // namespace ssp4sim::analysis