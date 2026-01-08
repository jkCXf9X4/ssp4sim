#pragma once

#include "ssp4sim_definitions.hpp"

#include "signal/storage.hpp"

#include "cutecpp/log.hpp"
#include "utils/model.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ssp4sim::graph
{
    struct ConnectionInfo : public types::IWritable
    {
        static Logger log;

        types::DataType type;
        size_t size;

        ssp4sim::signal::SignalStorage *source_storage;
        ssp4sim::signal::SignalStorage *target_storage;
        uint32_t source_index;
        uint32_t target_index;

        uint64_t delay = 0;

        bool forward_derivatives = false;
        int forward_derivatives_order = 0;

        std::string to_string() const override;

        static void retrieve_model_inputs(std::vector<ConnectionInfo> &connections,
                                          int target_area,
                                          uint64_t input_time);
    };
}
