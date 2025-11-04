#pragma once

#include "ssp4sim_definitions.hpp"

#include "cutecpp/log.hpp"
#include "utils/data_type.hpp"

#include <vector>

namespace ssp4sim::utils
{
    class RingStorage;
}

namespace ssp4sim::graph
{
    struct ConnectionInfo : public types::IPrintable
    {
        static Logger log;

        types::DataType type;
        size_t size;

        ssp4sim::utils::RingStorage *source_storage;
        ssp4sim::utils::RingStorage *target_storage;
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
