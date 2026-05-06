#pragma once

#include "signal/sinks/influx_recorder_common.hpp"
#include "signal/recorder.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

namespace ssp4sim::signal
{
    class InfluxRecorderSink final : public RecorderSink
    {
    public:
        ssp4cpp::utils::log::Logger *log = nullptr;

        InfluxRecordingConfig config;

        std::chrono::system_clock::time_point run_start_wall_clock{};

        std::unique_ptr<InfluxWriter> writer;
        std::vector<InfluxStorageLayout> layouts;
        std::unordered_map<const SignalStorage *, std::size_t> layout_lookup;

        bool initialized = false;
        bool disabled = false;
        bool started = false;


        InfluxRecorderSink(ssp4sim::InfluxRecordingConfig influx,
            std::chrono::system_clock::time_point run_start_wall_clock = {},
            std::unique_ptr<InfluxWriter> writer = nullptr);

        void on_storage_added(const SignalStorage *storage) override;

        void init() override;

        void start() override;

        void on_event(const NewDataEvent &event) override;

        void stop() override;

    private:
        void disable_sink(const std::string &reason);

        void ensure_run_start_initialized();

        bool should_record(InfluxStorageLayout &layout, std::uint64_t timestamp) const;

        std::string build_line_protocol(
            const InfluxStorageLayout &layout,
            const std::byte *data,
            double simulation_time_s,
            std::int64_t timestamp_ns) const;
    };
}
