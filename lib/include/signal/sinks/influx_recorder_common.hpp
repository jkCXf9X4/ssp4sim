#pragma once

#include "signal/storage.hpp"

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <stdexcept>
#include <system_error>
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

    std::string escape_measurement(std::string_view value);

    std::string escape_tag(std::string_view value);

    std::string escape_string_field(std::string_view value);

    void append_double(std::string &target, double value);

    template <typename T>
    void append_integral(std::string &target, T value)
    {
        char buffer[32];
        const auto [ptr, ec] = std::to_chars(buffer, buffer + sizeof(buffer), value);
        if (ec != std::errc{})
        {
            throw std::runtime_error("Failed to format integer for Influx line protocol");
        }
        target.append(buffer, ptr);
    }

    std::string with_nanosecond_precision(const std::string &url);

    std::string build_batch_payload(const std::vector<std::string> &pending, std::size_t pending_bytes);
}
