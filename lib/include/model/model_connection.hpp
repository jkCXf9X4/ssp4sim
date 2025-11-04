#pragma once

#include "utils/node.hpp"
#include "utils/map.hpp"
#include "utils/time.hpp"

#include "ssp4sim_definitions.hpp"

#include "FMI2_Enums_Ext.hpp"

#include "data_ring_storage.hpp"
#include "data_recorder.hpp"
#include "invocable.hpp"
#include "config.hpp"

#include "fmu_handler.hpp"

#include <string>
#include <vector>
#include <functional>

namespace ssp4sim::graph
{
    struct ConnectionInfo : public types::IPrintable
    {
        static Logger log;

        types::DataType type;
        size_t size;

        utils::RingStorage *source_storage;
        utils::RingStorage *target_storage;
        uint32_t source_index;
        uint32_t target_index;

        uint64_t delay = 0;

        bool forward_derivatives = false;
        int forward_derivatives_order = 0;

        void print(std::ostream &os) const override;

        static void retrieve_model_inputs(std::vector<ConnectionInfo> &connections,
                                          int target_area,
                                          uint64_t input_time);
    };
}
