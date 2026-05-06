#pragma once

#include "signal/recorder.hpp"

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ssp4sim::signal
{
    class InfluxWriter
    {
    public:
        virtual ~InfluxWriter() = default;

        virtual void batch_of(std::size_t size) = 0;

        virtual void write(std::string line) = 0;

        virtual void flush_batch() = 0;
    };

    struct InfluxVariableLayout
    {
        std::string name;
        types::DataType type;
        std::size_t position = 0;
        std::string line_prefix;
        bool string_value = false;
    };

    struct InfluxStorageLayout
    {
        const SignalStorage *storage = nullptr;
        std::size_t index = 0;
        std::vector<InfluxVariableLayout> variables;
        std::uint64_t last_recorded_timestamp = 0;
        bool has_recorded_timestamp = false;
    };

    class InfluxRecorderSink final : public RecorderSink
    {
    public:
        ssp4cpp::utils::log::Logger *log = nullptr;

        std::string url;
        std::string token;
        std::string measurement;
        std::string run_name;
        std::size_t batch_size = 50000;
        std::uint64_t recording_interval = 0;

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
            const InfluxVariableLayout &variable,
            const std::byte *data,
            double simulation_time_s,
            std::int64_t timestamp_ns) const;
    };
}
