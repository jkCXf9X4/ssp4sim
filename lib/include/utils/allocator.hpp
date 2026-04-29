#pragma once

#include "ssp4sim_definitions.hpp"

#include <cstddef>
#include <cstdlib>
#include <memory>
#include <new>
#include <vector>
#include <tuple>
#include <cstring>
#include <span>
#include <stdexcept>
#include <limits>

namespace ssp4sim::utils
{

    constexpr std::size_t target_alignment = alignof(std::max_align_t);

    constexpr std::size_t align_up(std::size_t value, std::size_t alignment)
    {
        return (value + alignment - 1) / alignment * alignment;
    }

    inline std::size_t get_value_alignment(types::DataType type)
    {
        switch (type)
        {
        case types::DataType::real:
            return alignof(double);
        case types::DataType::boolean:
        case types::DataType::integer:
        case types::DataType::enumeration:
            return alignof(int);
        case types::DataType::string:
            return alignof(std::string);
        case types::DataType::unknown:
            return alignof(std::byte);
        }
        return alignof(std::byte);
    }

    struct FreeDeleter
    {
        void operator()(std::byte *p) const noexcept
        {
            std::free(p);
        }
    };

    using aligned_unique_buffer = std::unique_ptr<std::byte[], FreeDeleter>;

    aligned_unique_buffer make_aligned_buffer(std::size_t size,
                                              std::size_t alignment = alignof(std::max_align_t));

    std::tuple<aligned_unique_buffer, std::size_t> make_aligned_buffers(std::size_t size,
                                                                        std::size_t capacity,
                                                                        std::size_t alignment = alignof(std::max_align_t));

    class AlignedBufferPool
    {
    public:
        std::size_t original_size;
        std::size_t aligned_size;
        std::size_t capacity = 0; /* total usable slots                 */

        std::size_t total_size;

        aligned_unique_buffer data;
        std::vector<std::byte *> locations; // absolute location in memory for each item
        std::vector<std::size_t> positions; // position relative start pointer for each item

        std::vector<std::span<const std::byte>> spans;

        AlignedBufferPool() = default;

        AlignedBufferPool(std::size_t size,
                          std::size_t capacity,
                          std::size_t alignment = alignof(std::max_align_t));

        void initialize();
    };
}
