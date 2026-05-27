#pragma once

#include "ssp4cpp/utils/log.hpp"

#include "ssp4sim_definitions.hpp"
#include "utils/allocator.hpp"

#include <cstddef>
#include <cstdint>
#include <atomic>
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
        std::vector<bool> used; // has the buffer been populated at least once

        AlignedBufferPool buffers;

        std::size_t head = 0;       /* current active position             */
        std::size_t capacity = 0;   /* total usable slots                 */
        std::size_t nr_inserts = 0; /* current number of elements stored  */
        std::size_t reserved_inserts = 0; /* writer-only sequence, may be ahead of published inserts */
        std::atomic<std::size_t> published_inserts = 0;

        RingBuffer(size_t capacity, size_t buffer_size);
        
        ~RingBuffer(){
            LOG_TRACE_L1(log, "Destroying RingBuffer");
        }

        // Get the index of the next item
        std::size_t push();

        std::size_t push(std::uint64_t time);

        // Single-writer reserve/commit path. Reserved slots are invisible to readers until committed.
        std::size_t reserve();

        void commit(std::size_t index);

        void commit(std::size_t index, std::uint64_t time);

        // get data from an index, index is static from data start
        std::byte *get_item(std::size_t index, bool use_verification=true);

        std::uint64_t get_time(std::size_t index);

        bool find_index(uint64_t time, std::size_t &index_found);

        bool find_latest_valid_index(uint64_t time, std::size_t &index_found);

        /*
        Return element at logical position `index` counting backwards from
        the head: index 0 == head, 1 == just before head, 2 == next-newest, …
        */
        std::size_t get_index_from_pos_rev(std::size_t position);

        bool is_empty();

        bool is_full();

        std::string to_string() const override;
    };
}
