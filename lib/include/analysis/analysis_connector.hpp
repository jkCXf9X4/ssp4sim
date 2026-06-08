#pragma once

#include "initial_value.hpp"
#include "ssp4sim_definitions.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

namespace ssp4sim::analysis
{

    class AnalysisConnector
    {
    public:
        std::string name;
        std::string type_str;  // "IO" or "P"
        types::Causality causality;
        bool is_boundary = false;

        int index = 0;
        uint64_t value_reference = 0;

        types::DataType data_type = types::DataType::unknown;
        std::size_t size = 0;
        std::unique_ptr<ext::ssp1::ssv::StartValue> initial_value = nullptr;

        bool forward_derivatives = false;
        int forward_derivatives_order = 0;
        bool is_feedthrough = false;

        AnalysisConnector() = default;

        AnalysisConnector(std::string component_name,
                          std::string connector_name_,
                          uint64_t value_reference_,
                          types::DataType data_type_);

        ~AnalysisConnector();

        AnalysisConnector(AnalysisConnector &&) = default;
        AnalysisConnector &operator=(AnalysisConnector &&) = default;

        AnalysisConnector(const AnalysisConnector &) = delete;
        AnalysisConnector &operator=(const AnalysisConnector &) = delete;

        static std::string create_name(const std::string &component_name,
                                        const std::string &connector_name_);

        std::string to_string() const;
    };

} // namespace ssp4sim::analysis