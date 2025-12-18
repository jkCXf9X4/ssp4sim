#include "utils/ring_buffer.hpp"

#include <cstring>
#include <stdexcept>

namespace ssp4sim::utils
{

    RingBuffer::RingBuffer(size_t items, size_t item_size) : timestamps(items)
    {
        log(ext_trace)("[{}] Constructor", __func__);
        if (items == 0)
        {
            throw std::runtime_error("[RingBuffer] buffer_size != 0");
        }
        this->capacity = items;

        auto total_size = items * item_size;

        data = std::make_unique<std::byte[]>(total_size);
        std::memset(data.get(), 0, total_size);

        for (size_t i = 0; i < items; i++)
        {
            auto position = item_size * i;
            locations.push_back(data.get() + position);
        }
    }

    std::size_t RingBuffer::push()
    {
        IF_LOG({
            log(ext_trace)("[{}] init", __func__);
        });

        if (is_full())
        {
            tail = (tail + 1) % capacity;
        }
        else
        {
            nr_items++;
        }

        head = (head + 1) % capacity;

        return head;
    }

    std::size_t RingBuffer::push(std::uint64_t time)
    {
        auto head = push();
        timestamps[head] = time;
        return head;
    }

    std::byte *RingBuffer::get_item(std::size_t index)
    {
        if (index >= capacity) [[unlikely]]
        {
            log(error)("[{}] RingBuffer, index out of range: {}", __func__, index);
            throw std::runtime_error("[RingBuffer][get_item] Index out of range");
        }
        return locations[index];
    }

    std::uint64_t RingBuffer::get_time(std::size_t index)
    {
        if (index >= capacity) [[unlikely]]
        {
            log(error)("[{}] RingBuffer, index out of range: {}", __func__, index);
            throw std::runtime_error("[RingBuffer][get_time] Index out of range");
        }
        return timestamps[index];
    }

    bool RingBuffer::find_index(uint64_t time, std::size_t &index_found)
    {
        for (std::size_t i = 0; i < nr_items; ++i)
        {
            int pos = get_index_from_pos_rev(i);
            if (timestamps[pos] == time)
            {
                index_found = pos;
                return true;
            }
        }
        return false;
    }

    bool RingBuffer::find_latest_valid_index(uint64_t time, std::size_t &index_found)
    {
        for (std::size_t i = 0; i < nr_items; ++i)
        {
            int pos = get_index_from_pos_rev(i);
            if (timestamps[pos] <= time)
            {
                IF_LOG({
                    log(ext_trace)("[{}] found valid area, {}", __func__, pos);
                });

                index_found = pos;
                return true;
            }
        }
        return false;
    }

    bool RingBuffer::is_empty()
    {
        return nr_items == 0;
    }

    bool RingBuffer::is_full()
    {
        return capacity == nr_items;
    }

    std::size_t RingBuffer::get_index_from_pos(std::size_t position)
    {
        return (tail + position) % capacity;
    }

    std::size_t RingBuffer::get_index_from_pos_rev(std::size_t position)
    {
        return (head + nr_items - position) % capacity;
    }

    void RingBuffer::print(std::ostream &os) const
    {
        os << "SignalStorage \n{\n"
           << ", capacity: " << capacity
           << "  nr_items: " << nr_items
           << ", head: " << head
           << ", tail: " << tail
           << "\n}";
    }

}
