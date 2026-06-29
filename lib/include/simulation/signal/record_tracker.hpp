#pragma once

#include "ssp4sim_definitions.hpp"

#include "signal/storage.hpp"
#include "utils/ring_buffer.hpp"

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

namespace ssp4sim::signal
{
    struct RecorderStorageBuffer
    {
        ssp4cpp::utils::log::Logger *log = nullptr;
        // Connected to one storage
        SignalStorage *storage = nullptr;

        // Each SignalStorage has one producer. The shared event queue handles
        // cross-storage MPSC fan-in; this buffer only guards recorder lag.
        std::size_t index = 0;
        std::atomic<std::size_t> active_items = 0;

        std::unique_ptr<utils::RingBuffer> buffers;

        RecorderStorageBuffer() = default;

        RecorderStorageBuffer(const RecorderStorageBuffer &) = delete;
        RecorderStorageBuffer &operator=(const RecorderStorageBuffer &) = delete;

        RecorderStorageBuffer(RecorderStorageBuffer &&other) noexcept
            : log(other.log),
              storage(other.storage),
              index(other.index),
              active_items(other.active_items.load(std::memory_order_relaxed)),
              buffers(std::move(other.buffers))
        {
        }

        RecorderStorageBuffer &operator=(RecorderStorageBuffer &&other) noexcept
        {
            if (this != &other)
            {
                log = other.log;
                storage = other.storage;
                index = other.index;
                active_items.store(other.active_items.load(std::memory_order_relaxed), std::memory_order_relaxed);
                buffers = std::move(other.buffers);
            }
            return *this;
        }

        RecorderStorageBuffer(SignalStorage *storage, std::size_t index, std::size_t capacity)
            : log(ssp4cpp::utils::log::make_logger("ssp4sim.signal.RecorderStorageBuffer"))
        {
            this->storage = storage;
            this->index = index;
            buffers = std::make_unique<utils::RingBuffer>(capacity, storage->mem_size);
        }

        bool try_push(std::size_t source_area, std::size_t &target_area)
        {
            auto previous_active_items = active_items.fetch_add(1, std::memory_order_acq_rel);
            if (previous_active_items >= buffers->capacity)
            {
                active_items.fetch_sub(1, std::memory_order_acq_rel);
                return false;
            }
            auto source = storage->data->get_item(source_area, false);
            auto source_time = storage->data->get_time(source_area);
            target_area = buffers->push(source_time);
            std::memcpy(buffers->get_item(target_area), source, buffers->buffers.aligned_size);

            return true;
        }

        void pop()
        {
            active_items.fetch_sub(1, std::memory_order_acq_rel);
        }
    };
}
