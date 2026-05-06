#pragma once

#include "signal/sinks/influx_recorder_common.hpp"

#include <string>
#include <vector>

namespace ssp4sim::signal
{
    class InfluxHttpWriter final : public InfluxWriter
    {
    public:
        explicit InfluxHttpWriter(std::string url, std::string token);

        void batch_of(std::size_t size) override;

        void write(std::string line) override;

        void flush_batch() override;

    private:
        std::string url;
        std::string token;
        std::size_t batch_size = 1;
        static constexpr std::size_t max_payload_bytes = 9 * 1024 * 1024;
        std::vector<std::string> pending;
        std::size_t pending_bytes = 0;

        void flush_pending();
    };
}
