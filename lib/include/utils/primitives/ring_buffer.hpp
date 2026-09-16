#pragma once

#include "ssp4cpp/utils/log.hpp"

#include "ssp4sim_definitions.hpp"
#include "utils/memory/allocator.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ssp4sim::utils
{

    /**
     * @brief Small ring buffer implementation
     * When full it will continuously overwrite the oldest data
     * Also tracks time of additions
     *
     */
    class RingBuffer : public types::IWritable
    {

    public:
        ssp4cpp::utils::log::Logger* log = nullptr;

        std::vector<std::uint64_t> timestamps;
        std::vector<bool> populated; // has the buffer been written at least once

        AlignedBufferPool buffers;

        std::size_t head = 0;         /* current active position             */
        std::size_t capacity = 0;     /* total usable slots                 */
        std::size_t write_count = 0;  /* cumulative number of pushes         */

        RingBuffer(size_t capacity, size_t buffer_size);
        
        ~RingBuffer(){
            LOG_TRACE_L1(log, "Destroying RingBuffer");
        }

        // Get the index of the next item
        std::size_t push();

        std::size_t push(std::uint64_t time);

        // get data from an index, index is static from data start
        std::byte *get_item(std::size_t index, bool use_verification=true);

        std::uint64_t get_time(std::size_t index);

        bool find_exact_index(uint64_t time, std::size_t &index_found);

        bool find_latest_valid_index(uint64_t time, std::size_t &index_found);

        /*
        Return element at logical position `index` counting backwards from
        the head: index 0 == head, 1 == just before head, 2 == next-newest, …
        */
        std::size_t index_back_from_head(std::size_t position);

        bool is_empty();

        bool is_full();

        /// True if slot \p index (physical) has been written at least once.
        bool is_populated(std::size_t index);

        /// Find the first stored timestamp strictly greater than \p time.
        /// Searches from newest to oldest. Returns true if found.
        bool find_next_valid_index(uint64_t time, std::size_t &index_found);

        std::string to_string() const override;
    };
}
