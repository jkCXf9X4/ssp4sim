#pragma once

#include "ssp4cpp/utils/log.hpp"

#include "ssp4sim_definitions.hpp"

#include "signal/storage.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace ssp4sim::graph
{
    /// How a connection reads its source data:
    ///  - Latest:    zero-order hold on the newest data at/ before the requested input time
    ///  - StartTime: sample the source at the start of the target model's step span
    ///  - EndTime:   sample the source at the end of the target model's step span
    enum class DataAccessMode : int
    {
        StartTime,
        EndTime,
        LatestTime,
        Index
    };

    struct ConnectionInfo : public types::IWritable
    {
        ConnectionInfo()
            : log(ssp4cpp::utils::log::make_logger("ssp4sim.ConnectionInfo"))
        {
        }
        ~ConnectionInfo(){
        };

        ssp4cpp::utils::log::Logger* log = nullptr;

        types::DataType type;
        size_t size;

        ssp4sim::signal::SignalStorage *source_storage;
        ssp4sim::signal::SignalStorage *target_storage;
        uint32_t source_index;
        uint32_t target_index;

        uint64_t delay = 0;

        DataAccessMode mode = DataAccessMode::LatestTime;
        int64_t time_offset = 0;
        int64_t fixed_index = 0;

        bool is_feedthrough = false;

        bool forward_derivatives = false;
        int forward_derivatives_order = 0;

        std::string to_string() const override;

        static void retrieve_model_inputs(std::vector<ConnectionInfo> &connections,
                                          int target_area,
                                          uint64_t input_time,
                                          uint64_t step_start,
                                          uint64_t step_end);
    };
}
