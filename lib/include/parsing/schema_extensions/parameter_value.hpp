

#pragma once

#include "ssp4sim_definitions.hpp"

#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace ssp4sim::ext
{
    using ParameterValueData = std::variant<std::monostate, double, int, std::string>;

    struct ParameterValue : public types::IWritable
    {
        std::string name;
        // std::vector<std::string> mappings; // name + mappings

        types::DataType type = types::DataType::unknown;
        std::size_t size;

        ParameterValueData value;

        ParameterValue(std::string name, types::DataType type);

        std::string to_string() const override;

        void store_value(void *value);

        void *raw_ptr();

        const void *raw_ptr() const;
    };
}
