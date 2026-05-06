#include "signal/sinks/influx_recorder_common.hpp"

#include <charconv>
#include <limits>
#include <sstream>
#include <stdexcept>

namespace ssp4sim::signal
{
    std::string escape_measurement(std::string_view value)
    {
        std::string escaped;
        escaped.reserve(value.size());
        for (char ch : value)
        {
            if (ch == ',' || ch == ' ')
            {
                escaped += '\\';
            }
            escaped += ch;
        }
        return escaped;
    }

    std::string escape_tag(std::string_view value)
    {
        std::string escaped;
        escaped.reserve(value.size());
        for (char ch : value)
        {
            if (ch == ',' || ch == ' ' || ch == '=')
            {
                escaped += '\\';
            }
            escaped += ch;
        }
        return escaped;
    }

    std::string escape_string_field(std::string_view value)
    {
        std::string escaped;
        escaped.reserve(value.size() + 2);
        escaped += '"';
        for (char ch : value)
        {
            if (ch == '"' || ch == '\\')
            {
                escaped += '\\';
            }
            escaped += ch;
        }
        escaped += '"';
        return escaped;
    }

    void append_double(std::string &target, double value)
    {
        std::ostringstream stream;
        stream.precision(std::numeric_limits<double>::max_digits10);
        stream << value;
        if (!stream)
        {
            throw std::runtime_error("Failed to format floating-point value for Influx line protocol");
        }
        target += stream.str();
    }

    std::string with_nanosecond_precision(const std::string &url)
    {
        if (url.find("precision=") != std::string::npos)
        {
            return url;
        }

        if (url.find('?') == std::string::npos)
        {
            return url + "?precision=ns";
        }

        return url + "&precision=ns";
    }

    std::string build_batch_payload(const std::vector<std::string> &pending, std::size_t pending_bytes)
    {
        std::string body;
        body.reserve(pending_bytes == 0 ? 0 : pending_bytes - 1);
        for (std::size_t i = 0; i < pending.size(); ++i)
        {
            if (i > 0)
            {
                body += '\n';
            }
            body += pending[i];
        }
        return body;
    }

}
