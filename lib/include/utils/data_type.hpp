#pragma once

#include "ssp4cpp/schema/fmi2/FMI2_Enums.hpp"

#include "handler/fmi4c_adapter.hpp"

#include "ssp4sim_definitions.hpp"

#include <cstddef>
#include <string>
#include <stdexcept>

namespace ssp4sim::utils
{
    void read_from_model_(types::DataType t,
                          handler::CoSimulationModel &model,
                          uint64_t value_reference,
                          void *out);

    void write_to_model_(types::DataType t,
                         handler::CoSimulationModel &model,
                         uint64_t &value_reference,
                         void *data);

}
