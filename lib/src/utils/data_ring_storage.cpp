#include "utils/data_ring_storage.hpp"

#include "utils/data_storage.hpp"

#include <cstring>
#include <stdexcept>
#include <utility>

namespace ssp4sim::utils
{

    RingStorage::RingStorage(size_t capacity, std::string name) : name(std::move(name))
    {
        log(ext_trace)("[{}] Constructor", __func__);
        if (capacity == 0)
        {
            throw std::runtime_error("[RingBuffer] buffer_size != 0");
        }
        this->capacity = capacity;

        data = std::make_unique<DataStorage>(static_cast<int>(capacity), this->name);
    }

    uint64_t RingStorage::add(std::string name, types::DataType type, int max_interpolation_order)
    {
        return data->add(std::move(name), type, max_interpolation_order);
    }

    void RingStorage::allocate()
    {
        data->allocate();
    }

    int RingStorage::push(uint64_t time)
    {
        auto area = push();
        data->set_time(static_cast<std::size_t>(area), time);
        return area;
    }

    int RingStorage::get_or_push(uint64_t time)
    {
        auto area = get_area(time);
        if (area != -1)
        {
            return static_cast<int>(area);
        }
        auto new_area = push();
        data->set_time(static_cast<std::size_t>(new_area), time);
        return new_area;
    }

    int64_t RingStorage::get_area(uint64_t time)
    {
        IF_LOG({
            log(ext_trace)("[{}] init", __func__);
        });

        for (std::size_t i = 0; i < size; ++i)
        {
            int pos = static_cast<int>(get_pos_rev(static_cast<int>(i)));
            if (data->timestamps[static_cast<std::size_t>(pos)] == time)
            {
                IF_LOG({
                    log(ext_trace)("[{}] found valid area, {}", __func__, pos);
                });

                return pos;
            }
        }
        return -1;
    }

    int64_t RingStorage::get_valid_area(uint64_t time)
    {
        IF_LOG({
            log(ext_trace)("[{}] init", __func__);
        });

        for (std::size_t i = 0; i < size; ++i)
        {
            int pos = static_cast<int>(get_pos_rev(static_cast<int>(i)));
            if (data->timestamps[static_cast<std::size_t>(pos)] <= time)
            {
                IF_LOG({
                    log(ext_trace)("[{}] found valid area, {}", __func__, pos);
                });

                return pos;
            }
        }
        return -1;
    }

    byte *RingStorage::get_item(std::size_t area, std::size_t index)
    {
        return data->get_item(area, index);
    }

    byte *RingStorage::get_derivative(std::size_t area, std::size_t index, std::size_t order)
    {
        return data->get_derivative(area, index, order);
    }

    byte *RingStorage::get_valid_item(uint64_t time, std::size_t index)
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

            return data->get_item(static_cast<std::size_t>(valid_area), index);
        }
        return nullptr;
    }

    void RingStorage::flag_new_data(std::size_t area)
    {
        data->flag_new_data(area);
    }

    bool RingStorage::is_empty()
    {
        return size == 0;
    }

    bool RingStorage::is_full()
    {
        return size == capacity;
    }

    int RingStorage::push()
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
            size++;
        }

        head = (head + 1) % capacity;

        if (data->new_data_flags[head])
        {
            overwrite_counter += 1;
        }

        return static_cast<int>(head);
    }

    uint64_t RingStorage::get_pos(int index)
    {
        return (tail + static_cast<std::size_t>(index)) % capacity;
    }

    uint64_t RingStorage::get_pos_rev(int index)
    {
        return (head + capacity - static_cast<std::size_t>(index)) % capacity;
    }

    void RingStorage::print(std::ostream &os) const
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

}
