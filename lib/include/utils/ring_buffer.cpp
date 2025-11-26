#include "utils/ring_buffer.hpp"

#include <cstring>
#include <stdexcept>
#include <utility>

namespace ssp4sim::utils
{

    RingBuffer::RingBuffer(size_t items, size_t item_size)
    {
        log(ext_trace)("[{}] Constructor", __func__);
        if (items == 0)
        {
            throw std::runtime_error("[RingBuffer] buffer_size != 0");
        }
        this->capacity = items;

        auto total_size = items * item_size;

        data = std::make_unique<std::byte[]>(total_size);

        for (size_t i = 0; i < items; i++)
        {
            auto position = item_size *i;
            positions.push_back(0);
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

    std::byte *RingBuffer::get_item(std::size_t index)
    {
        if (index >= capacity)
        {
            log(error)("[{}] Ringbuffer, index outr of range: {}", __func__, index);
            throw std::runtime_error("[RingBuffer] Index out of range");
        }
        return locations[index];
    }

    bool RingBuffer::is_empty()
    {
        return nr_items == 0;
    }

    bool RingBuffer::is_full()
    {
        return capacity == nr_items;
    }

    std::size_t RingBuffer::get_index(std::size_t position)
    {
        return (tail + position) % capacity;
    }

    std::size_t RingBuffer::get_index_rev(std::size_t position)
    {
        return (head + nr_items - position) % capacity;
    }

    void RingBuffer::print(std::ostream &os) const
    {
        os << "RingStorage \n{\n"
           << ", capacity: " << capacity
           << "  nr_items: " << nr_items
           << ", head: " << head
           << ", tail: " << tail
           << "\n}";
    }

}
