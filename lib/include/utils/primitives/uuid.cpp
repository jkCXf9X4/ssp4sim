#include "utils/primitives/uuid.hpp"

#include <array>
#include <cstdint>
#include <format>
#include <random>

namespace ssp4sim::utils
{
    std::string make_uuid_v4()
    {
        // 16 random bytes for a 128-bit UUID
        std::array<std::uint8_t, 16> bytes{};
        {
            std::random_device rd;
            for (auto &b : bytes)
            {
                b = static_cast<std::uint8_t>(rd());
            }
        }

        // Set version bits (4 most significant bits of byte 6): 0100
        bytes[6] = (bytes[6] & 0x0f) | 0x40;

        // Set variant bits (2 most significant bits of byte 8): 10
        bytes[8] = (bytes[8] & 0x3f) | 0x80;

        // Format as 8-4-4-4-12 hex string
        return std::format(
            "{:02x}{:02x}{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}-{:02x}{:02x}{:02x}{:02x}{:02x}{:02x}",
            bytes[0], bytes[1], bytes[2], bytes[3],
            bytes[4], bytes[5],
            bytes[6], bytes[7],
            bytes[8], bytes[9],
            bytes[10], bytes[11], bytes[12], bytes[13], bytes[14], bytes[15]);
    }
}