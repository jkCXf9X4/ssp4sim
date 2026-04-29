#include "utils/allocator.hpp"

namespace ssp4sim::utils
{

    aligned_unique_buffer make_aligned_buffer(std::size_t size,
                                              std::size_t alignment)
    {
        std::size_t aligned_size = align_up(size, alignment);

        void *ptr = std::aligned_alloc(alignment, aligned_size);

        if (!ptr)
        {
            throw std::bad_alloc{};
        }

        std::memset(ptr, 0, aligned_size);

        return aligned_unique_buffer{static_cast<std::byte *>(ptr)};
    }

    std::tuple<aligned_unique_buffer, std::size_t> make_aligned_buffers(std::size_t size,
                                                                        std::size_t capacity,
                                                                        std::size_t alignment )
    {
        std::size_t aligned_size = align_up(size, alignment);
        if (capacity != 0 && aligned_size > std::numeric_limits<std::size_t>::max() / capacity)
        {
            throw std::overflow_error("allocation size overflow");
        }

        std::size_t total_size = aligned_size * capacity;

        void *ptr = std::aligned_alloc(alignment, total_size);
        if (!ptr)
        {
            throw std::bad_alloc{};
        }
        std::memset(ptr, 0, total_size);

        return {
            aligned_unique_buffer{static_cast<std::byte *>(ptr)},
            aligned_size};
    }

    AlignedBufferPool::AlignedBufferPool(std::size_t size,
                                         std::size_t capacity,
                                         std::size_t alignment)
    {
        this->original_size = size;
        this->capacity = capacity;

        auto [d, as] = make_aligned_buffers(this->original_size, this->capacity, alignment);
        this->data = std::move(d);
        this->aligned_size = as;
        this->total_size = this->capacity * this->aligned_size;

        this->initialize();
    }

    void AlignedBufferPool::initialize()
    {
        this->positions.clear();
        this->locations.clear();
        this->spans.clear();

        for (size_t i = 0; i < this->capacity; i++)
        {
            auto position = this->aligned_size * i;
            this->positions.push_back(position);
            this->locations.push_back(data.get() + position);
            this->spans.push_back({data.get() + position, this->aligned_size});
        }
    }

}
