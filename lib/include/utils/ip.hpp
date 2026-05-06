#pragma once

#include <string>
#include <string_view>
#include <stdexcept>
#include <utility>

namespace ssp4sim::utils::ip
{

    static std::pair<std::string, std::string> parse_host_port(const std::string &value)
    {
        const auto colon_pos = value.rfind(':');
        if (colon_pos == std::string::npos || colon_pos == 0 || colon_pos + 1 >= value.size())
        {
            throw std::runtime_error("must be in the format host:port");
        }

        return {
            value.substr(0, colon_pos),
            value.substr(colon_pos + 1),
        };
    }
}
