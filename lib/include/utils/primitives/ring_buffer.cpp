#include "utils/primitives/ring_buffer.hpp"

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

        nr_inserts += 1;
        head = nr_inserts % capacity;
        used[head] = true;

        return head;
    }

    std::size_t RingBuffer::push(std::uint64_t time)
    {
        // assign new time before increasing nr_inserts and head
        // doing the opposite gives a chance that head points to a very old time that will fullfill the find_latest_valid_index and return something that is wrong
        // timestamps[nr_inserts + 1% capacity] = time;
        timestamps[(nr_inserts+1) % capacity] = time;
        auto head = push();
        return head;
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
        // store local_nr_inserts before iteration since there is a risk of it changing during the loop risking missing or processing an item twice
        auto local_nr_inserts = nr_inserts;
        for (std::size_t i = 0; i < local_nr_inserts && i < capacity; ++i)
        {
            int pos = (local_nr_inserts - i) % capacity;
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
        // store local_nr_inserts before iteration since there is a risk of it changing during the loop risking missing or processing an item twice

        auto local_nr_inserts = nr_inserts;
        for (std::size_t i = 0; i < local_nr_inserts && i < capacity; ++i)
        {
            int pos = (local_nr_inserts - i) % capacity;
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
        return (nr_inserts - position) % capacity;
    }

    bool RingBuffer::is_empty()
    {
        return nr_inserts == 0;
    }

    bool RingBuffer::is_full()
    {
        return nr_inserts >= capacity;
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
