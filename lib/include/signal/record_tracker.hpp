#pragma once

#include "ssp4sim_definitions.hpp"

#include "signal/storage.hpp"

#include <atomic>
#include <string>


namespace ssp4sim::signal
{
    struct Tracker
    {
        SignalStorage *storage;

        std::size_t size = 0;
        std::size_t index = 0;
        std::size_t row_pos = 0;

        std::atomic<uint64_t> timestamp = 0;

        Tracker() = default;

        Tracker(const Tracker &other)
            : storage(other.storage),
              size(other.size),
              index(other.index),
              row_pos(other.row_pos),
              timestamp(other.timestamp.load())
        {
        }

        Tracker &operator=(const Tracker &other)
        {
            if (this != &other)
            {
                storage = other.storage;
                size = other.size;
                index = other.index;
                row_pos = other.row_pos;
                timestamp.store(other.timestamp.load());
            }
            return *this;
        }

        Tracker(Tracker &&other) noexcept
            : storage(other.storage),
              size(other.size),
              index(other.index),
              row_pos(other.row_pos),
              timestamp(other.timestamp.load())
        {
        }

        Tracker &operator=(Tracker &&other) noexcept
        {
            if (this != &other)
            {
                storage = other.storage;
                size = other.size;
                index = other.index;
                row_pos = other.row_pos;
                timestamp.store(other.timestamp.load());
            }
            return *this;
        }
    };
}
