
#pragma once

#include "ssp4cpp/schema/fmi2/FMI2_Enums.hpp"

#include "ssp4sim_definitions.hpp"

#include <cstddef>
#include <string>

namespace ssp4sim::ext::fmi2
{

    namespace enums
    {

        namespace default_values
        {
            const bool default_bool = false;
            const double default_real = 0.0;
            const int default_int = 0;
            const std::string default_string = "";
        }

        /**
         * @brief  Return the in-memory size (in bytes) of a single value
         *         represented by the given DataType.
         */
        std::size_t get_data_type_size(types::DataType t);

        std::string data_type_to_string(types::DataType type, void *data);

        // this contains dangerous solution... remove if possible
        void *get_default_value(types::DataType type);
    }

}
