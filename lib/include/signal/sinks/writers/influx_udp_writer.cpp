#include "signal/sinks/writers/influx_udp_writer.hpp"

#include <cerrno>
#include <cstring>
#include <netdb.h>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>

namespace ssp4sim::signal
{
    namespace
    {
        std::string socket_error_message()
        {
            return std::strerror(errno);
        }
    }

    UdpInfluxWriter::UdpInfluxWriter(std::string host, std::string port)
        : host(std::move(host)),
          port(std::move(port))
    {
        connect_socket();
    }

    UdpInfluxWriter::~UdpInfluxWriter()
    {
        if (socket_fd != -1)
        {
            ::close(socket_fd);
        }
    }

    void UdpInfluxWriter::batch_of(std::size_t size)
    {
        batch_size = size == 0 ? 1 : size;
        pending.reserve(batch_size);
    }

    void UdpInfluxWriter::write(std::string line)
    {
        const auto projected_bytes = pending.empty() ? line.size() : pending_bytes + 1 + line.size();
        if (!pending.empty() && projected_bytes > max_payload_size)
        {
            flush_pending();
        }

        pending_bytes = pending.empty() ? line.size() : pending_bytes + 1 + line.size();
        pending.emplace_back(std::move(line));

        if (pending.size() >= batch_size || pending_bytes >= max_payload_size)
        {
            flush_pending();
        }
    }

    void UdpInfluxWriter::flush_batch()
    {
        flush_pending();
    }

    void UdpInfluxWriter::connect_socket()
    {
        addrinfo hints{};
        hints.ai_family = AF_UNSPEC;
        hints.ai_socktype = SOCK_DGRAM;
        hints.ai_protocol = IPPROTO_UDP;

        addrinfo *result = nullptr;
        const auto rc = ::getaddrinfo(host.c_str(), port.c_str(), &hints, &result);
        if (rc != 0 || result == nullptr)
        {
            throw std::runtime_error("Failed to resolve UDP target " + host + ":" + port + ": " + ::gai_strerror(rc));
        }

        for (auto *entry = result; entry != nullptr; entry = entry->ai_next)
        {
            auto fd = ::socket(entry->ai_family, entry->ai_socktype, entry->ai_protocol);
            if (fd == -1)
            {
                continue;
            }

            if (::connect(fd, entry->ai_addr, static_cast<int>(entry->ai_addrlen)) == 0)
            {
                socket_fd = fd;
                ::freeaddrinfo(result);
                return;
            }

            ::close(fd);
        }

        ::freeaddrinfo(result);
        throw std::runtime_error("Failed to connect UDP target " + host + ":" + port + ": " + socket_error_message());
    }

    void UdpInfluxWriter::flush_pending()
    {
        if (pending.empty())
        {
            return;
        }

        const auto body = build_batch_payload(pending, pending_bytes);
        send_body(body);

        pending.clear();
        pending_bytes = 0;
    }

    void UdpInfluxWriter::send_body(const std::string &body)
    {
        if (body.empty())
        {
            return;
        }

        const auto sent = ::send(socket_fd, body.data(), body.size(), 0);
        if (sent < 0)
        {
            throw std::runtime_error("Influx UDP write failed: " + socket_error_message());
        }

        if (static_cast<std::size_t>(sent) != body.size())
        {
            throw std::runtime_error("Influx UDP write was truncated");
        }
    }
}
