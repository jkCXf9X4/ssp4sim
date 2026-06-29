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
        AnalysisModel *model;

        types::Causality causality;

        uint64_t value_reference = 0;

        types::DataType data_type = types::DataType::unknown;
        std::size_t size = 0;
        std::unique_ptr<ext::ParameterValue> initial_value;

        AnalysisConnector() = default;

        AnalysisConnector(std::string connector_name_,
                          uint64_t value_reference_,
                          types::DataType data_type_,
                          types::Causality causality_);

        ~AnalysisConnector();

        AnalysisConnector(AnalysisConnector &&) = default;
        AnalysisConnector &operator=(AnalysisConnector &&) = default;

        AnalysisConnector(const AnalysisConnector &) = delete;
        AnalysisConnector &operator=(const AnalysisConnector &) = delete;

        std::string to_string() const;
    };

} // namespace ssp4sim::analysis