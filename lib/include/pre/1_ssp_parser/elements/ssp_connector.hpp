#pragma once

#include "_ssp_item.hpp"

#include "../schema_extensions/parameter_value.hpp"
#include "ssp4sim_definitions.hpp"
#include "ssp4cpp/schema/fmi2/FMI2_modelDescription.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace ssp4sim::analysis
{

    class SspConnector : public SspItem
    {
    public:
        types::Causality causality;

        uint64_t value_reference = 0;

        types::DataType data_type = types::DataType::unknown;

        ext::ParameterValue initial_value;

        std::vector<ssp4cpp::fmi2::md::fmi2ScalarVariable *> dependencies;

        SspConnector(std::string connector_name_,
                          uint64_t value_reference_,
                          types::DataType data_type_,
                          types::Causality causality_);

        std::string to_string() const;
    };

} // namespace ssp4sim::analysis