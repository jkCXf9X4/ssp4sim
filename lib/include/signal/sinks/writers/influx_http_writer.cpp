#include "signal/sinks/writers/influx_http_writer.hpp"

#include <cpr/cpr.h>

#include <stdexcept>
#include <utility>

namespace ssp4sim::signal
{
    InfluxHttpWriter::InfluxHttpWriter(std::string url, std::string token)
        : url(with_nanosecond_precision(url)),
          token(std::move(token))
    {
    }

    void InfluxHttpWriter::batch_of(std::size_t size)
    {
        batch_size = size == 0 ? 1 : size;
        pending.reserve(batch_size);
    }

    void InfluxHttpWriter::write(std::string line)
    {
        const auto line_bytes = line.size() + 1;

        if (!pending.empty() && pending_bytes + line_bytes > max_payload_bytes)
        {
            flush_pending();
        }

        pending_bytes += line_bytes;
        pending.emplace_back(std::move(line));

        if (pending.size() >= batch_size || pending_bytes >= max_payload_bytes)
        {
            flush_pending();
        }
    }

    void InfluxHttpWriter::flush_batch()
    {
        flush_pending();
    }

    void InfluxHttpWriter::flush_pending()
    {
        if (pending.empty())
        {
            return;
        }

        const auto body = build_batch_payload(pending, pending_bytes);

        cpr::Header headers{{"Content-Type", "text/plain; charset=utf-8"}};
        if (!token.empty())
        {
            headers.emplace("Authorization", "Bearer " + token);
        }

        const auto response = cpr::Post(
            cpr::Url{url},
            headers,
            cpr::Body{body},
            cpr::Timeout{5000},
            cpr::ConnectTimeout{1000});

        if (response.error.code != cpr::ErrorCode::OK)
        {
            throw std::runtime_error("Influx write failed: " + response.error.message);
        }

        if (response.status_code < 200 || response.status_code >= 300)
        {
            throw std::runtime_error("Influx write failed with HTTP status " + std::to_string(response.status_code) + ": " + response.text);
        }

        pending.clear();
        pending_bytes = 0;
    }
}
