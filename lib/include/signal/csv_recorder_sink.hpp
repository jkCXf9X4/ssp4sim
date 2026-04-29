#pragma once

#include "signal/recorder.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace ssp4sim::signal
{
    struct CsvTracker
    {
        SignalStorage *storage = nullptr;
        std::size_t index = 0;
        std::size_t row_pos = 0;
    };

    class CsvRecorderSink final : public RecorderSink
    {
    public:
        ssp4cpp::utils::log::Logger* log = nullptr;

        std::ofstream file;
        std::vector<CsvTracker> trackers;

        std::uint16_t head = 0;
        std::size_t new_item_counter = 0;
        const std::size_t rows = 500;

        std::size_t row_size = 0;

        std::unordered_map<std::uint64_t, std::uint64_t> row_time_map;
        std::unordered_map<std::uint64_t, std::uint64_t> time_row_map;
        std::unique_ptr<std::byte[]> data;

        uint64_t recording_interval = 0;
        uint64_t last_print_time = 0;
        size_t printed_rows = 0;

        CsvRecorderSink(const std::string &filename, uint64_t interval);

        void on_storage_added(const SignalStorage *storage) override;

        void init() override;

        void on_event(const NewDataEvent &event) override;

        void stop() override;

        void reset_update_status(std::size_t row);

        void print_headers();

        std::byte *get_data_pos(std::size_t row, std::size_t offset);

        void print_row(uint16_t row);
    };
}
