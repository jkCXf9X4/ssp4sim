#include "utils/ring_buffer.hpp"

#include <cstring>
#include <sstream>
#include <stdexcept>

namespace ssp4sim::utils
{

    // There are almost no bound checks in this class
    // use with care...

    RingBuffer::RingBuffer(size_t capacity, size_t item_size) : timestamps(capacity), used(capacity)
    {
        log(ext_trace)("[{}] Constructor", __func__);
        if (capacity == 0)
        {
            throw std::runtime_error("[RingBuffer] buffer_size != 0");
        }
        this->capacity = capacity;

        auto total_size = this->capacity * item_size;

        data = std::make_unique<std::byte[]>(total_size);
        std::memset(data.get(), 0, total_size);

        for (size_t i = 0; i < this->capacity; i++)
        {
            auto position = item_size * i;
            locations.push_back(data.get() + position);
            used[i] = false;
        }
    }

    std::size_t RingBuffer::push()
    {
        IF_LOG({
            log(ext_trace)("[{}] init", __func__);
        });
        
        nr_inserts += 1;
        head = nr_inserts % capacity;
        used[head] = true;

        return head;
    }

    std::size_t RingBuffer::push(std::uint64_t time)
    {
        auto head = push();
        timestamps[head] = time;
        return head;
    }

    std::byte *RingBuffer::get_item(std::size_t index, bool use_verification)
    {
        if (use_verification && !used[index]) [[unlikely]]
        {
            log(error)("[{}] RingBuffer, index not populated: {}", __func__, index);
            throw std::runtime_error("[RingBuffer][get_item] Index not populated");
        }
        return locations[index];
    }

    std::uint64_t RingBuffer::get_time(std::size_t index)
    {
        if (!used[index]) [[unlikely]]
        {
            log(error)("[{}] RingBuffer, index not populated: {}", __func__, index);
            throw std::runtime_error("[RingBuffer][get_time] Index not populated");
        }
        return timestamps[index];
    }

    bool RingBuffer::find_index(uint64_t time, std::size_t &index_found)
    {
        for (std::size_t i = 0;i < nr_inserts && i < capacity ; ++i)
        {
            int pos = get_index_from_pos_rev(i);
            if (timestamps[pos] == time)
            {
                IF_LOG({
                    log(trace)("[{}] found valid index, {}", __func__, pos);
                });

                index_found = pos;
                return true;
            }
        }
        return false;
    }

    bool RingBuffer::find_latest_valid_index(uint64_t time, std::size_t &index_found)
    {
        for (std::size_t i = 0; i < nr_inserts && i < capacity; ++i)
        {
            int pos = get_index_from_pos_rev(i);
            if (timestamps[pos] <= time)
            {
                IF_LOG({
                    log(trace)("[{}] found valid area, {}", __func__, pos);
                });

                index_found = pos;
                return true;
            }
        }
        return false;
    }

    bool RingBuffer::is_empty()
    {
        return nr_inserts == 0;
    }

    bool RingBuffer::is_full()
    {
        return nr_inserts >= capacity;
    }

    std::size_t RingBuffer::get_index_from_pos_rev(std::size_t position)
    {
        return (nr_inserts - position) % capacity;
    }

    std::string RingBuffer::to_string() const
    {
        std::ostringstream oss;
        oss << "SignalStorage \n{\n"
            << ", capacity: " << capacity
            << "  nr_inserts: " << nr_inserts
            << ", head: " << head
            << "\n}";
        return oss.str();
    }

}
