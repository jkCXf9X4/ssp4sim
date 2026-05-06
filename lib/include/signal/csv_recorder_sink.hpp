#pragma once

#include "signal/recorder.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>
#include <filesystem>

namespace ssp4sim::signal
{
    struct CsvVariableLayout
    {
        std::string name;
        types::DataType type;
        std::size_t position = 0;
        std::size_t column = 0;
    };

    struct CsvStorageLayout
    {
        const SignalStorage *storage = nullptr;
        std::size_t index = 0;
        std::vector<CsvVariableLayout> variables;
    };

    struct CsvRow
    {
        bool valid = false;
        std::uint64_t timestamp = 0;
        std::vector<std::string> values;
        std::vector<bool> updated_storages;
    };

    class CsvRecorderSink final : public RecorderSink
    {
    public:
        ssp4cpp::utils::log::Logger *log = nullptr;

        std::ofstream file;
        std::vector<CsvStorageLayout> layouts;
        std::unordered_map<const SignalStorage *, std::size_t> layout_lookup;

        std::uint16_t head = 0;
        std::size_t new_item_counter = 0;
        const std::size_t rows = 500;

        std::size_t column_count = 0;

        std::unordered_map<std::uint16_t, std::uint64_t> row_time_map;
        std::unordered_map<std::uint64_t, std::uint16_t> time_row_map;
        std::vector<CsvRow> row_buffer;

        std::uint64_t recording_interval = 0;
        std::uint64_t last_recorded_timestamp = 0;
        bool has_recorded_timestamp = false;
        std::size_t printed_rows = 0;

        CsvRecorderSink(const std::filesystem::path &filename, std::uint64_t interval);

        void on_storage_added(const SignalStorage *storage) override;

        void init() override;

        void on_event(const NewDataEvent &event) override;

        void stop() override;

        void reset_row(std::uint16_t row);

        void print_headers();

        void print_row(std::uint16_t row);

        std::uint16_t row_for_timestamp(std::uint64_t timestamp);
    };
}
