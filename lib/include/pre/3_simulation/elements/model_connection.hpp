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
    /// A typed connection between two signal storages. This is the *wire fact*:
    /// WHAT the edge is (source/target/type/indices/delay/derivatives). Sampling
    /// policy (mode/offset) is NOT a field here — it is owned by the read-target
    /// resolver (ssp4sim::scheduling::detail::Edge), which derives it from graph
    /// facts and schedule context.
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

        bool is_feedthrough = false;

        bool forward_derivatives = false;
        int forward_derivatives_order = 0;

        std::string to_string() const override;
    };
}
