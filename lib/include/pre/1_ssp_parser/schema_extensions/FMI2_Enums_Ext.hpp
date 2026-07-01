
#pragma once

#include "ssp4cpp/schema/fmi2/FMI2_Enums.hpp"

#include "ssp4sim_definitions.hpp"

#include <cstddef>
#include <string>
#include <variant>

namespace ssp4sim::ext::fmi2
{

    namespace enums
    {

        using DefaultValue = std::variant<
            bool,
            int,
            double,
            std::string_view>;

        /**
         * @brief  Return the in-memory size (in bytes) of a single value
         *         represented by the given DataType.
         */
        std::size_t get_data_type_size(types::DataType t);

        std::string data_type_to_string(types::DataType type, void *data);

        DefaultValue get_default_value(types::DataType type);
    }

}
