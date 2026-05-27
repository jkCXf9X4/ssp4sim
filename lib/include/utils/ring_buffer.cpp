#include "utils/ring_buffer.hpp"

#include <cstring>
#include <sstream>
#include <stdexcept>

namespace ssp4sim::utils
{

    // There are almost no bound checks in this class
    // use with care...

    RingBuffer::RingBuffer(size_t capacity, size_t buffer_size)
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.utils.RingBuffer")),
          timestamps(capacity),
          used(capacity)
    {
        LOG_TRACE_L2(log, "[{func}] Constructor", __func__);
        if (capacity == 0)
        {
            throw std::runtime_error("[RingBuffer] buffer_size != 0");
        }
        this->capacity = capacity;
        
        buffers = AlignedBufferPool(buffer_size, this->capacity);
        
        for (size_t i = 0; i < this->capacity; i++)
        {
            used[i] = false;
        }
    }

    std::size_t RingBuffer::push()
    {
        IF_LOG({
            LOG_TRACE_L2(log, "[{func}] init", __func__);
        });

        auto index = reserve();
        commit(index);
        return index;
    }

    std::size_t RingBuffer::push(std::uint64_t time)
    {
        auto index = reserve();
        commit(index, time);
        return index;
    }

    std::size_t RingBuffer::reserve()
    {
        if (reserved_inserts != nr_inserts) [[unlikely]]
        {
            throw std::runtime_error("[RingBuffer][reserve] Previous reservation has not been committed");
        }
        reserved_inserts += 1;
        return reserved_inserts % capacity;
    }

    void RingBuffer::commit(std::size_t index)
    {
        used[index] = true;
        head = index;
        nr_inserts = reserved_inserts;
        published_inserts.store(nr_inserts, std::memory_order_release);
    }

    void RingBuffer::commit(std::size_t index, std::uint64_t time)
    {
        timestamps[index] = time;
        commit(index);
    }

    std::byte *RingBuffer::get_item(std::size_t index, bool use_verification)
    {
        if (use_verification && !used[index]) [[unlikely]]
        {
            LOG_ERROR(log, "[{func}] RingBuffer, index not populated: {index}", __func__, index);
            throw std::runtime_error("[RingBuffer][get_item] Index not populated");
        }
        return buffers.locations[index];
    }

    std::uint64_t RingBuffer::get_time(std::size_t index)
    {
        if (!used[index]) [[unlikely]]
        {
            LOG_ERROR(log, "[{func}] RingBuffer, index not populated: {index}", __func__, index);
            throw std::runtime_error("[RingBuffer][get_time] Index not populated");
        }
        return timestamps[index];
    }

    bool RingBuffer::find_index(uint64_t time, std::size_t &index_found)
    {
        const auto inserts = published_inserts.load(std::memory_order_acquire);
        for (std::size_t i = 0; i < inserts && i < capacity; ++i)
        {
            auto pos = (inserts - i) % capacity;
            if (!used[pos])
            {
                continue;
            }
            if (timestamps[pos] == time)
            {
                IF_LOG({
                    LOG_TRACE_L1(log, "[{func}] found valid index, {index}", __func__, pos);
                });

                index_found = pos;
                return true;
            }
        }
        return false;
    }

    bool RingBuffer::find_latest_valid_index(uint64_t time, std::size_t &index_found)
    {
        const auto inserts = published_inserts.load(std::memory_order_acquire);
        for (std::size_t i = 0; i < inserts && i < capacity; ++i)
        {
            auto pos = (inserts - i) % capacity;
            if (!used[pos])
            {
                continue;
            }
            if (timestamps[pos] <= time)
            {
                IF_LOG({
                    LOG_TRACE_L1(log, "[{func}] found valid area, {}", __func__, pos);
                });

                index_found = pos;
                return true;
            }
        }
        return false;
    }

    std::size_t RingBuffer::get_index_from_pos_rev(std::size_t position)
    {
        const auto inserts = published_inserts.load(std::memory_order_acquire);
        return (inserts - position) % capacity;
    }

    bool RingBuffer::is_empty()
    {
        return published_inserts.load(std::memory_order_acquire) == 0;
    }

    bool RingBuffer::is_full()
    {
        return published_inserts.load(std::memory_order_acquire) >= capacity;
    }

    std::string RingBuffer::to_string() const
    {
        std::ostringstream oss;
        const auto inserts = published_inserts.load(std::memory_order_acquire);
        oss << "SignalStorage \n{\n"
            << ", capacity: " << capacity
            << "  nr_inserts: " << inserts
            << ", head: " << head
            << "\n}";
        return oss.str();
    }

}
