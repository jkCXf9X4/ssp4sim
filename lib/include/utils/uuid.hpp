#pragma once

#include <string>

namespace ssp4sim::utils
{
    /**
     * @brief Generate a RFC 4122 UUIDv4 string.
     *
     * Format: xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx
     * Version bits: 0100 (version 4)
     * Variant bits:  10 (RFC 4122)
     */
    std::string make_uuid_v4();
}