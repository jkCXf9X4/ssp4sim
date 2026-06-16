#pragma once

#include "analysis_component.hpp"

#include "initial_value.hpp"
#include "ssp4sim_definitions.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace ssp4sim::analysis
{

    class AnalysisModel;

    class AnalysisConnector : public AnalysisComponent
    {
    public:
        std::string component_name;
        std::string connector_name;
        bool is_boundary = false;
        bool is_feedthrough = false;
        bool forward_derivatives = false;
        int forward_derivatives_order = 0;

        types::Causality causality;

        uint64_t value_reference = 0;

        types::DataType data_type = types::DataType::unknown;
        std::size_t size = 0;
        std::unique_ptr<ext::ssp1::ssv::StartValue> initial_value = nullptr;

        AnalysisConnector() = default;

        AnalysisConnector(std::string component_name_,
                          std::string connector_name_,
                          uint64_t value_reference_,
                          types::DataType data_type_);

        ~AnalysisConnector();

        AnalysisConnector(AnalysisConnector &&) = default;
        AnalysisConnector &operator=(AnalysisConnector &&) = default;

        AnalysisConnector(const AnalysisConnector &) = delete;
        AnalysisConnector &operator=(const AnalysisConnector &) = delete;

        /// Build a fully qualified connector name from component and connector names.
        static std::string get_connector_name(const std::string &component,
                                              const std::string &connector);

        /// Alias for get_connector_name.
        static std::string create_name(const std::string &component,
                                       const std::string &connector)
        {
            return get_connector_name(component, connector);
        }

        std::string to_string() const;
    };

} // namespace ssp4sim::analysis