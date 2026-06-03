

#pragma once

#include "ssp4sim_definitions.hpp"

#include <sstream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace ssp4sim::ext::ssp1::ssv
{
    using StartValueData = std::variant<std::monostate, double, int, std::string>;

    struct StartValue : public types::IWritable
    {
        std::string name;
        std::vector<std::string> mappings; // name + mappings
        types::DataType type = types::DataType::unknown;
        StartValueData value;

        StartValue(std::string name, types::DataType type);

        std::string to_string() const override;

        void store_value(void *value);

        void *raw_ptr();

        const void *raw_ptr() const;
    };
}
