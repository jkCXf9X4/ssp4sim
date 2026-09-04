#include "utils/primitives/ring_buffer.hpp"

#include <cstring>
#include <sstream>
#include <stdexcept>

namespace ssp4sim::utils
{

    // There are almost no bound checks in this class
    // use with care...

    /**
     * @brief Constructor: allocate a fixed-capacity ring of byte slots.
     *
     * Sizes the timestamp/populated vectors to \p capacity and allocates one
     * contiguous aligned block of `capacity * buffer_size` bytes via
     * AlignedBufferPool. Every slot starts unused (no data written yet).
     *
     * @param capacity    number of slots
     * @param buffer_size bytes per slot
     * @throws std::runtime_error if capacity == 0
     */
    RingBuffer::RingBuffer(size_t capacity, size_t buffer_size)
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.utils.RingBuffer")),
          timestamps(capacity),
          populated(capacity)
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
            populated[i] = false;
        }
    }

    /**
     * @brief Advance to the next slot, marking it populated, without a timestamp.
     *
     * Increments write_count, wraps `head = write_count % capacity`, marks that
     * slot as populated, and returns the new head index.
     */
    std::size_t RingBuffer::push()
    {
        IF_LOG({
            LOG_TRACE_L2(log, "[{func}] init", __func__);
        });

        write_count += 1;
        head = write_count % capacity;
        populated[head] = true;

        return head;
    }

    /**
     * @brief Advance to the next slot and stamp it with \p time.
     *
     * Writes \p time into the slot ABOUT to become head *before* advancing
     * head. Doing it the other way round lets head briefly point at a very old
     * timestamp, causing find_latest_valid_index to return a stale area.
     */
    std::size_t RingBuffer::push(std::uint64_t time)
    {
        // assign new time before increasing write_count and head
        // doing the opposite gives a chance that head points to a very old time that will fullfill the find_latest_valid_index and return something that is wrong
        // timestamps[write_count + 1% capacity] = time;
        timestamps[(write_count+1) % capacity] = time;
        auto head = push();
        return head;
    }

    /**
     * @brief Return the raw byte pointer for slot \p index.
     *
     * @param index slot to read/write
     * @param use_verification if true (default), reject unpopulated slots
     * @throws std::runtime_error if verification on and slot never written
     */
    std::byte *RingBuffer::get_item(std::size_t index, bool use_verification)
    {
        if (use_verification && !populated[index]) [[unlikely]]
        {
            LOG_ERROR(log, "[{func}] RingBuffer, index not populated: {index}", __func__, index);
            throw std::runtime_error("[RingBuffer][get_item] Index not populated");
        }
        return buffers.locations[index];
    }

    /**
     * @brief Timestamp recorded for slot \p index.
     * @throws std::runtime_error if the slot was never written.
     */
    std::uint64_t RingBuffer::get_time(std::size_t index)
    {
        if (!populated[index]) [[unlikely]]
        {
            LOG_ERROR(log, "[{func}] RingBuffer, index not populated: {index}", __func__, index);
            throw std::runtime_error("[RingBuffer][get_time] Index not populated");
        }
        return timestamps[index];
    }

    /**
     * @brief Exact timestamp match.
     * @return true and set \p index_found to the newest slot with time == \p time,
     *         else false.
     *
     * Scans newest-first; write_count is snapshotted locally so a concurrent
     * push cannot make the loop miss or double-process a slot.
     */
    bool RingBuffer::find_exact_index(uint64_t time, std::size_t &index_found)
    {
        // store local_write_count before iteration since there is a risk of it changing during the loop risking missing or processing an item twice
        auto local_write_count = write_count;
        for (std::size_t i = 0; i < local_write_count && i < capacity; ++i)
        {
            int pos = (local_write_count - i) % capacity;
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

    /**
     * @brief Newest slot whose time is <= \p time.
     * @return true and set \p index_found, else false if none qualifies.
     */
    bool RingBuffer::find_latest_valid_index(uint64_t time, std::size_t &index_found)
    {
        // store local_write_count before iteration since there is a risk of it changing during the loop risking missing or processing an item twice

        auto local_write_count = write_count;
        for (std::size_t i = 0; i < local_write_count && i < capacity; ++i)
        {
            int pos = (local_write_count - i) % capacity;
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

    /**
     * @brief Map a logical backwards position to a physical slot.
     * position 0 == head (newest), 1 == just before head, 2 == two back, ...
     */
    std::size_t RingBuffer::index_back_from_head(std::size_t position)
    {
        return (write_count - position) % capacity;
    }

    /** @brief True if no slot has ever been written. */
    bool RingBuffer::is_empty()
    {
        return write_count == 0;
    }

    /** @brief True once the buffer has wrapped at least once. */
    bool RingBuffer::is_full()
    {
        return write_count >= capacity;
    }

    /** @brief Human-readable summary of capacity / inserts / head. */
    std::string RingBuffer::to_string() const
    {
        std::ostringstream oss;
        oss << "SignalStorage \n{\n"
            << ", capacity: " << capacity
            << "  write_count: " << write_count
            << ", head: " << head
            << "\n}";
        return oss.str();
    }


/**
     * @brief Newest slot whose time is strictly greater than \p time.
     * @return true and set \p index_found, else false if none qualifies.
     *
     * Searches newest-first; inverse of find_latest_valid_index.
     */
bool RingBuffer::find_next_valid_index(uint64_t time, std::size_t &index_found)
    {
        auto local_write_count = write_count;
        for (std::size_t i = 0; i < local_write_count && i < capacity; ++i)
        {
            int pos = (local_write_count - i) % capacity;
            if (timestamps[pos] > time)
            {
                IF_LOG({
                    LOG_TRACE_L1(log, "[{func}] found next valid index, {index}", __func__, pos);
                });

                index_found = pos;
                return true;
            }
        }
        return false;
    }
}

    