#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace ssp4sim::signal
{
    template <typename T>
    class BoundedMpscEventQueue
    {
    public:
        explicit BoundedMpscEventQueue(std::size_t capacity)
            : buffer(capacity),
              capacity(capacity)
        {
            if (capacity == 0)
            {
                throw std::runtime_error("[BoundedMpscEventQueue] capacity must be greater than zero");
            }

            for (std::size_t i = 0; i < capacity; ++i)
            {
                buffer[i].sequence.store(i, std::memory_order_relaxed);
            }
        }

        bool try_push(const T &item)
        {
            Cell *cell = nullptr;
            auto position = enqueue_position.load(std::memory_order_relaxed);

            while (true)
            {
                cell = &buffer[position % capacity];
                const auto sequence = cell->sequence.load(std::memory_order_acquire);
                const auto difference = static_cast<std::ptrdiff_t>(sequence) - static_cast<std::ptrdiff_t>(position);

                if (difference == 0)
                {
                    if (enqueue_position.compare_exchange_weak(position, position + 1, std::memory_order_relaxed))
                    {
                        break;
                    }
                }
                else if (difference < 0)
                {
                    return false;
                }
                else
                {
                    position = enqueue_position.load(std::memory_order_relaxed);
                }
            }

            cell->data = item;
            cell->sequence.store(position + 1, std::memory_order_release);
            return true;
        }

        bool try_pop(T &item)
        {
            auto &cell = buffer[dequeue_position % capacity];
            const auto sequence = cell.sequence.load(std::memory_order_acquire);
            const auto difference = static_cast<std::ptrdiff_t>(sequence) - static_cast<std::ptrdiff_t>(dequeue_position + 1);
            if (difference != 0)
            {
                return false;
            }

            item = cell.data;
            cell.sequence.store(dequeue_position + capacity, std::memory_order_release);
            dequeue_position += 1;
            return true;
        }

    private:
        struct Cell
        {
            std::atomic<std::size_t> sequence = 0;
            T data{};
        };

        std::vector<Cell> buffer;
        std::size_t capacity = 0;

        alignas(64) std::atomic<std::size_t> enqueue_position = 0;
        alignas(64) std::size_t dequeue_position = 0;
    };
}
