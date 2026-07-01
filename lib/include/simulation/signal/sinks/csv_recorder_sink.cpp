#include "signal/sinks/csv_recorder_sink.hpp"

#include "pre/1_ssp_parser/schema_extensions/FMI2_Enums_Ext.hpp"
#include "utils/time/time.hpp"
#include "utils/io/io.hpp"

#include <algorithm>
#include <utility>
#include <filesystem>


namespace ssp4sim::signal
{
    CsvRecorderSink::CsvRecorderSink(const std::filesystem::path &filename, std::uint64_t interval)
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.signal.CsvRecorderSink")),
          recording_interval(interval)
    {
        utils::io::create_parent_folder(filename.string());
        file.open(filename, std::ios::out);

        LOG_DEBUG(log, "[{func}] File {file}, open {open}", __func__, filename.string(), file.is_open());
        LOG_DEBUG(log, "[{func}] Interval: {interval}", __func__, recording_interval);
    }

    void CsvRecorderSink::on_storage_added(const SignalStorage *storage)
    {
        if (storage == nullptr || storage->mem_size == 0)
        {
            return;
        }

        CsvStorageLayout layout;
        layout.storage = storage;
        layout.index = layouts.size();
        layout.variables.reserve(storage->variables.size());

        for (const auto &variable : storage->variables)
        {
            CsvVariableLayout variable_layout;
            variable_layout.name = variable.name;
            variable_layout.type = variable.type;
            variable_layout.position = variable.position;
            variable_layout.column = column_count++;
            layout.variables.emplace_back(std::move(variable_layout));
        }

        layout_lookup[storage] = layout.index;
        layouts.emplace_back(std::move(layout));
    }

    void CsvRecorderSink::init()
    {
        LOG_TRACE_L1(log, "[{func}] Init", __func__);
        row_buffer.clear();
        row_buffer.resize(rows);

        for (auto &row : row_buffer)
        {
            row.values.resize(column_count);
            row.updated_storages.resize(layouts.size());
        }

        print_headers();
    }

    void CsvRecorderSink::print_headers()
    {
        LOG_TRACE_L1(log, "[{func}] Printing headers", __func__);
        file << "time";
        for (const auto &layout : layouts)
        {
            for (const auto &variable : layout.variables)
            {
                file << ',' << variable.name;
            }
        }
        file << '\n';
        file.flush();
    }

    void CsvRecorderSink::reset_row(std::uint16_t row)
    {
        auto &target = row_buffer[row];
        target.valid = false;
        target.timestamp = 0;
        std::fill(target.values.begin(), target.values.end(), std::string{});
        std::fill(target.updated_storages.begin(), target.updated_storages.end(), false);
    }

    std::uint16_t CsvRecorderSink::row_for_timestamp(std::uint64_t timestamp)
    {
        if (auto found = time_row_map.find(timestamp); found != time_row_map.end())
        {
            return found->second;
        }

        head = static_cast<std::uint16_t>((head + 1) % rows);

        if (new_item_counter >= rows && row_buffer[head].valid)
        {
            print_row(head);
            time_row_map.erase(row_buffer[head].timestamp);
            row_time_map.erase(head);
            reset_row(head);
        }
        else
        {
            new_item_counter++;
        }

        auto &row = row_buffer[head];
        row.valid = true;
        row.timestamp = timestamp;
        row_time_map[head] = timestamp;
        time_row_map[timestamp] = head;

        return head;
    }

    void CsvRecorderSink::on_event(const NewDataEvent &event)
    {
        if (event.storage == nullptr || event.buffer == nullptr)
        {
            return;
        }

        auto layout_it = layout_lookup.find(event.storage);
        if (layout_it == layout_lookup.end())
        {
            LOG_WARNING(log, "[{func}] Ignoring event for unknown storage {}", __func__, event.storage->name);
            return;
        }

        const auto &layout = layouts[layout_it->second];

        std::uint16_t row_index = 0;
        if (const auto row_it = time_row_map.find(event.timestamp); row_it != time_row_map.end())
        {
            row_index = row_it->second;
        }
        else
        {
            if (recording_interval > 0 && has_recorded_timestamp && event.timestamp >= last_recorded_timestamp && event.timestamp - last_recorded_timestamp < recording_interval)
            {
                return;
            }

            row_index = row_for_timestamp(event.timestamp);
            last_recorded_timestamp = std::max(last_recorded_timestamp, event.timestamp);
            has_recorded_timestamp = true;
        }

        auto &row = row_buffer[row_index];

        for (const auto &variable : layout.variables)
        {
            row.values[variable.column] = ext::fmi2::enums::data_type_to_string(variable.type, event.buffer + variable.position);
        }
        row.updated_storages[layout.index] = true;
    }

    void CsvRecorderSink::print_row(std::uint16_t row)
    {
        const auto &source = row_buffer[row];
        if (!source.valid)
        {
            return;
        }

        file << utils::time::ns_to_s(source.timestamp);
        for (const auto &value : source.values)
        {
            file << ", " << value;
        }
        file << '\n';

        printed_rows += 1;
        if (printed_rows % 50 == 0)
        {
            file.flush();
        }
    }

    void CsvRecorderSink::stop()
    {
        for (std::size_t i = 1; i <= rows; i++)
        {
            auto row = static_cast<std::uint16_t>((head + i) % rows);
            if (row_buffer[row].valid)
            {
                print_row(row);
            }
        }

        file.flush();
        if (file.is_open())
        {
            file.close();
        }
    }
}
