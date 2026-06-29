#pragma once

#include "analysis_component.hpp"

#include "parameter_value.hpp"
#include "ssp4sim_definitions.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace ssp4sim::analysis
{

    class AnalysisModel
    {
        std::string name;
    };

    class AnalysisConnector : public AnalysisComponent
    {
    public:
        types::Causality causality;

        uint64_t value_reference = 0;

        types::DataType data_type = types::DataType::unknown;

        ext::ParameterValue initial_value;

        AnalysisConnector(std::string connector_name_,
                          uint64_t value_reference_,
                          types::DataType data_type_,
                          types::Causality causality_);

        std::string to_string() const;
    };

} // namespace ssp4sim::analysis