#pragma once

#include "cutecpp/log.hpp"

#include "ssp4sim_definitions.hpp"

#include "signal/storage.hpp"

#include <string>
#include <cstring>
#include <vector>
#include <deque>
#include <map>
#include <stdexcept>
#include <iostream>

#include <memory>
#include <cstddef>

namespace ssp4sim::utils
{

    /**
     * @brief Small ring buffer implementation
     * When full it will continuously overwrite the oldest data
     *
     */
    class RingBuffer : public types::IPrintable
    {

        Logger log = Logger("ssp4sim.utils.RingStorage", LogLevel::debug);

    public:
        std::size_t item_size = 0;
        std::unique_ptr<std::byte[]> data;
        std::vector<std::size_t> positions; // data position relative to start; 0, 4,...
        std::vector<std::byte *> locations; // absolute location in memory for each item

        std::size_t head = 1; /* next position to write             */
        std::size_t tail = 0; /* first data position      */

        std::size_t capacity = 0; /* total usable slots                 */
        std::size_t nr_items = 0; /* current number of elements stored  */

        RingBuffer(size_t items, size_t item_size);

        // Get the index of the next item
        std::size_t push();

        // get data from an index
        std::byte *get_item(std::size_t index);

        /* Return element at logical position `index` from the tail (oldest)     */
        std::size_t get_index(std::size_t position);

        /* Return element at logical position `index` counting backwards from
        the head: index 0 == head, 1 == just before head, 2 == next-newest, …   */
        std::size_t get_index_rev(std::size_t position);

        bool is_empty();

        bool is_full();

        virtual void print(std::ostream &os) const override;
    };
}
