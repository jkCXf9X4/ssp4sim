#pragma once

#include "cutecpp/log.hpp"

#include "ssp4sim_definitions.hpp"

#include "data_storage.hpp"

#include <string>
#include <cstring>
#include <vector>
#include <deque>
#include <map>
#include <stdexcept>
#include <iostream>

namespace ssp4sim::utils
{
    using namespace std;

    /**
     * @brief Small ring buffer implementation used for timestamped data.
     * When full it will continuously overwrite the oldest data
     * The buffer is not designed to store all data of the simulation but will continuously overwrite old data
     *
     */
    class RingStorage : public types::IPrintable
    {

        Logger log = Logger("ssp4sim.utils.RingStorage", LogLevel::debug);

    public:
        std::unique_ptr<DataStorage> data;

        size_t head = 0; /* next position to write             */
        size_t tail = 0; /* first data position      */

        size_t capacity = 0;            /* total usable slots                 */
        size_t size = 0;                /* current number of elements stored  */
        uint64_t overwrite_counter = 0; // how many times has new data been added

        std::string name;

        RingStorage(size_t capacity, std::string name);

        uint64_t add(std::string name, types::DataType type, int max_interpolation_order);

        void allocate();

        int push(uint64_t time);

        int get_or_push(uint64_t time);

        int64_t get_area(uint64_t time);

        /** Retrieve the most recent element with timestamp before @p time. */
        int64_t get_valid_area(uint64_t time);

        byte *get_item(std::size_t area, std::size_t index);

        byte *get_derivative(std::size_t area, std::size_t index, std::size_t order);

        byte *get_valid_item(uint64_t time, std::size_t index);

        void flag_new_data(std::size_t area);

    private:
        bool is_empty();

        bool is_full();

        // create new, if full it will overwrite the oldest data
        int push();

        /* Return element at logical position `index` from the tail (oldest)     */
        uint64_t get_pos(int index);

        /* Return element at logical position `index` counting backwards from
        the head: index 0 == head, 1 == just before head, 2 == next-newest, …   */
        uint64_t get_pos_rev(int index);

        virtual void print(std::ostream &os) const override;
    };
}
