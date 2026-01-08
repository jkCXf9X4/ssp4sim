#pragma once

#include "cutecpp/log.hpp"

#include "ssp4sim_definitions.hpp"

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

        Logger log = Logger("ssp4sim.utils.RingBuffer", LogLevel::debug);

    public:
        std::size_t item_size = 0;
        std::unique_ptr<std::byte[]> data;
        std::vector<std::uint64_t> timestamps;

        std::vector<std::byte *> locations; // absolute location in memory for each item

        std::size_t head = 0; /* current active position             */
        std::size_t tail = 0; /* first data position      */

        std::size_t capacity = 0; /* total usable slots                 */
        std::size_t nr_items = 0; /* current number of elements stored  */

        RingBuffer(size_t items, size_t item_size);

        // Get the index of the next item
        std::size_t push();

        std::size_t push(std::uint64_t time);

        // get data from an index, index is static from data start
        std::byte *get_item(std::size_t index);

        std::uint64_t get_time(std::size_t index);

        bool find_index(uint64_t time, std::size_t &index_found);

        bool find_latest_valid_index(uint64_t time, std::size_t &index_found);

        /* Return element at logical position `index` from the tail (oldest)     */
        std::size_t get_index_from_pos(std::size_t position);

        /* Return element at logical position `index` counting backwards from
        the head: index 0 == head, 1 == just before head, 2 == next-newest, …   */
        std::size_t get_index_from_pos_rev(std::size_t position);

        bool is_empty();

        bool is_full();

        std::string to_string() const override;
    };
}
