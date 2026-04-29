#include "signal/csv_recorder_sink.hpp"

#include "FMI2_Enums_Ext.hpp"

#include "utils/time.hpp"

#include <cstring>
#include <memory>
#include <utility>

namespace ssp4sim::signal
{
    CsvRecorderSink::CsvRecorderSink(const std::string &filename, uint64_t interval)
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.record.CsvRecorderSink")),
          file(filename, std::ios::out),
          recording_interval(interval)
    {
        LOG_DEBUG(log, "[{func}] File {file}, open {open}", __func__, filename, file.is_open());
        LOG_DEBUG(log, "[{func}] Interval: {interval}", __func__, recording_interval);
    }

    void CsvRecorderSink::on_storage_added(const RecorderStorageBuffer &tracker)
    {
        if (tracker.storage->mem_size == 0)
        {
            return;
        }

        CsvTracker copy;
        copy.storage = tracker.storage;
        copy.index = tracker.index;
        copy.row_pos = row_size;
        trackers.emplace_back(std::move(copy));
        row_size += tracker.storage->mem_size;
    }

    void CsvRecorderSink::reset_update_status(std::size_t row)
    {
        LOG_TRACE_L1(log, "[{func}] Init", __func__);
        for (auto &t : trackers)
        {
            if (t.index < updated_tracker[row].size())
            {
                updated_tracker[row][t.index] = false;
            }
        }
    }

    void CsvRecorderSink::print_headers()
    {
        LOG_TRACE_L1(log, "[{func}] Init", __func__);
        file << "time";
        for (const auto &tracker : trackers)
        {
            for (const auto &var : tracker.storage->variables)
            {
                file << ',' << var.name;
            }
        }
        file << '\n';
        file.flush();
    }

    void CsvRecorderSink::init()
    {
        LOG_TRACE_L1(log, "[{func}] Init", __func__);
        const auto allocation_size = row_size * rows;

        data = std::make_unique<std::byte[]>(allocation_size);
        LOG_TRACE_L1(log, "[{func}] Completed allocation", __func__);

        const std::size_t cols = trackers.size();

        updated_tracker.clear();
        updated_tracker.reserve(rows);
        for (std::size_t i = 0; i < rows; ++i)
        {
            updated_tracker.emplace_back(cols, false);
        }

        print_headers();
    }

    std::byte *CsvRecorderSink::get_data_pos(std::size_t row, std::size_t offset)
    {
        return data.get() + row * row_size + offset;
    }

    void CsvRecorderSink::print_row(uint16_t row)
    {
        IF_LOG({
            LOG_TRACE_L1(log, "[{func}] Row: {}", __func__, row);
        });

        auto time_value = utils::time::ns_to_s(row_time_map[row]);

        file << time_value;
        for (const auto &tracker : trackers)
        {
            for (auto& var : tracker.storage->variables)
            {
                IF_LOG({
                    LOG_TRACE_L2(log, "[{func}] Printing tracker: {}, item:{}", __func__, tracker.storage->name, var.name);
                });

                auto pos = var.position;
                auto type = var.type;
                file << ", ";
                if (tracker.index < updated_tracker[row].size() && updated_tracker[row][tracker.index])
                {
                    auto data_type_str = ssp4sim::ext::fmi2::enums::data_type_to_string(type, get_data_pos(row, tracker.row_pos + pos));
                    file << data_type_str;
                }
            }
        }
        file << '\n';
        printed_rows += 1;

        if (printed_rows % 50 == 0)
        {
            file.flush();
        }
    }

    void CsvRecorderSink::on_event(const NewDataEvent &event, const RecorderStorageBuffer &tracker)
    {
        auto ts = event.timestamp;
        if (tracker.index >= trackers.size())
        {
            LOG_WARNING(log, "[{func}] Ignoring event for unknown tracker index {}", __func__, tracker.index);
            return;
        }

        if (!time_row_map.contains(ts))
        {
            IF_LOG({
                LOG_TRACE_L1(log, "[{func}] New print time: {}, last_print_time {}", __func__, ts, last_print_time);
            });

            last_print_time += recording_interval;

            head = static_cast<uint16_t>((head + 1) % rows);

            if (new_item_counter >= rows) [[likely]]
            {
                IF_LOG({
                    LOG_TRACE_L1(log, "[{func}] Row already in use, print and reset. {}", __func__, head);
                });

                print_row(head);
                reset_update_status(head);
                time_row_map.erase(row_time_map[head]);
            }

            new_item_counter++;

            row_time_map[head] = ts;
            time_row_map[ts] = head;
            IF_LOG({
                LOG_TRACE_L1(log, "[{func}] New row {} with time {}", __func__, head, ts);
            });
        }

        if (time_row_map.contains(ts))
        {
            auto row = time_row_map[ts];
            IF_LOG({
                LOG_TRACE_L1(log, "[{func}] Copying new data; row {}, size: {}", __func__, row, tracker.storage->mem_size);
            });

            std::memcpy(get_data_pos(row, trackers[tracker.index].row_pos), event.buffer, tracker.storage->mem_size);
            updated_tracker[row][tracker.index] = true;
        }
    }

    void CsvRecorderSink::stop()
    {
        for (int i = 1; i <= rows; i++)
        {
            bool print = false;
            auto row = static_cast<uint16_t>((head + i) % rows);
            for (auto &tracker : trackers)
            {
                if (tracker.index < updated_tracker[row].size() && updated_tracker[row][tracker.index])
                {
                    print = true;
                }
            }
            if (print)
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
