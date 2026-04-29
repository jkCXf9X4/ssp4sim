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

        std::size_t index = 0;
        // TOdo: accessed from multiple threads, should be atomic
        std::size_t active_items = 0;

        std::unique_ptr<utils::RingBuffer> buffers;

        RecorderStorageBuffer() = default;

        RecorderStorageBuffer(SignalStorage *storage, std::size_t index, std::size_t capacity)
            : log(ssp4cpp::utils::log::make_logger("ssp4sim.signal.RecorderStorageBuffer"))
        {
            this->storage = storage;
            this->index = index;
            buffers = std::make_unique<utils::RingBuffer>(capacity, storage->mem_size);
        }

        bool try_push(std::size_t source_area, std::size_t &target_area)
        {
            if (active_items >= buffers->capacity)
            {
                return false;
            }
            active_items++;
            auto source = storage->data->get_item(source_area, false);
            auto source_time = storage->data->get_time(source_area);
            target_area = buffers->push(source_time);
            std::memcpy(buffers->get_item(target_area), source, buffers->buffers.aligned_size);

            return true;
        }

        void pop()
        {
            active_items--;
        }
    };
}
