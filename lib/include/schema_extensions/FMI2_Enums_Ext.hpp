
#pragma once

#include "ssp4cpp/schema/fmi2/FMI2_Enums.hpp"

#include "ssp4sim_definitions.hpp"

#include <string>

namespace ssp4sim::ext::fmi2
{

    namespace enums
    {
        /**
         * @brief  Return the in-memory size (in bytes) of a single value
         *         represented by the given DataType.
         */
        std::size_t get_data_type_size(types::DataType t);

        std::string data_type_to_string(types::DataType type, void *data);
    }

}
