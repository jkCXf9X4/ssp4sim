#pragma once

#include "utils/string.hpp"
#include "utils/log.hpp"

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
    class RingStorage : public ssp4cpp::utils::str::IString
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

        RingStorage(size_t capacity, std::string name) : name(name)
        {
            log(ext_trace)("[{}] Constructor", __func__);
            if (capacity == 0)
            {
                throw runtime_error("[RingBuffer] buffer_size != 0");
            }
            this->capacity = capacity;

            data = make_unique<DataStorage>(capacity, name);
        }

        uint64_t add(std::string name, utils::DataType type, int max_interpolation_order)
        {
            return data->add(name, type, max_interpolation_order);
        }

        void allocate()
        {
            data->allocate();
        }

        int push(uint64_t time)
        {
            auto area = push();
            data->set_time(area, time);
            return area;
        }

        int get_or_push(uint64_t time)
        {
            auto area = get_area(time);
            if (area != -1)
            {
                return area;
            }
            else
            {
                auto area = push();
                data->set_time(area, time);
                return area;
            }
        }

        int64_t get_area(uint64_t time)
        {
            IF_LOG({
                log(ext_trace)("[{}] init", __func__);
            });

            for (std::size_t i = 0; i < size; ++i)
            {
                int pos = get_pos_rev(i);
                if (data->timestamps[pos] == time)
                {
                    IF_LOG({
                        log(ext_trace)("[{}] found valid area, {}", __func__, pos);
                    });

                    return pos;
                }
            }
            return -1;
        }

        /** Retrieve the most recent element with timestamp before @p time. */
        int64_t get_valid_area(uint64_t time)
        {
            IF_LOG({
                log(ext_trace)("[{}] init", __func__);
            });

            for (std::size_t i = 0; i < size; ++i)
            {
                int pos = get_pos_rev(i);
                if (data->timestamps[pos] <= time)
                {
                    IF_LOG({
                        log(ext_trace)("[{}] found valid area, {}", __func__, pos);
                    });

                    return pos;
                }
            }
            return -1;
        }

        byte *get_item(std::size_t area, std::size_t index)
        {
            return data->get_item(area, index);
        }

        byte *get_derivative(std::size_t area, std::size_t index, std::size_t order)
        {
            return data->get_derivative(area, index, order);
        }

        byte *get_valid_item(uint64_t time, std::size_t index)
        {
            IF_LOG({
                log(ext_trace)("[{}] Init", __func__);
            });

            auto valid_area = get_valid_area(time);
            if (valid_area != -1)
            {
                IF_LOG({
                    log(ext_trace)("[{}] Valid area found, returning the pointer", __func__);
                });

                return data->get_item(valid_area, index);
            }
            return nullptr;
        }

        void flag_new_data(std::size_t area)
        {
            data->flag_new_data(area);
        }

    private:
        inline bool is_empty()
        {
            return size == 0;
        }

        inline bool is_full()
        {
            return size == capacity;
        }

        // create new, if full it will overwrite the oldest data
        int push()
        {
            IF_LOG({
                log(ext_trace)("[{}] init", __func__);
            });

            if (is_full()) [[likely]]
            {
                tail = (tail + 1) % capacity;
            }
            else [[unlikely]]
            {
                size++;
            }

            // increase head
            head = (head + 1) % capacity;

            // Fix sometime when you need to feel more sad ;)
            if (data->new_data_flags[head] == true)
            {
                overwrite_counter += 1;
            }

            return head;
        }

        /* Return element at logical position `index` from the tail (oldest)     */
        inline uint64_t get_pos(int index)
        {
            return (tail + index) % capacity;
        }

        /* Return element at logical position `index` counting backwards from
        the head: index 0 == head, 1 == just before head, 2 == next-newest, …   */
        inline uint64_t get_pos_rev(int index)
        {
            return (head + capacity - index) % capacity;
        }

        virtual void print(std::ostream &os) const
        {
            os << "RingStorage \n{\n"
               << "  capacity: " << capacity
               << ", size: " << size
               << ", head: " << head
               << ", tail: " << tail
               << ", overwrite_counter: " << overwrite_counter
               << "\n  " << *data
               << "\n}";
        }
    };
}