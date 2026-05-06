#pragma once

#include "signal/sinks/influx_recorder_common.hpp"

#include <string>
#include <vector>

namespace ssp4sim::signal
{
    class UdpInfluxWriter final : public InfluxWriter
    {
    public:
        UdpInfluxWriter(std::string host, std::string port);
        ~UdpInfluxWriter() override;

        void batch_of(std::size_t size) override;

        void write(std::string line) override;

        void flush_batch() override;

    private:
        std::string host;
        std::string port;
        int socket_fd = -1;
        std::size_t batch_size = 1;
        std::vector<std::string> pending;
        std::size_t pending_bytes = 0;
        static constexpr std::size_t max_payload_size = 1400;

        void connect_socket();

        void flush_pending();

        void send_body(const std::string &body);
    };
}
