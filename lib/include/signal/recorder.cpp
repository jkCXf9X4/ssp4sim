#include "signal/recorder.hpp"

#include "signal/storage.hpp"
#include "utils/time.hpp"

#include "config.hpp"

#include "FMI2_Enums_Ext.hpp"

#include <cstddef>
#include <cstring>
#include <memory>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

namespace ssp4sim::signal
{

    DataRecorder::DataRecorder(const std::string &filename, uint64_t interval, bool wait_for)
        : file(filename, std::ios::out)
    {
        LOG_TRACE_L2(log, "[{}] Constructor", __func__);
        LOG_DEBUG(log, "[{}] Recording interval {}", __func__, recording_interval);
        LOG_DEBUG(log, "[{}] File {}, open {}", __func__, filename, file.is_open());
        LOG_DEBUG(log, "[{}] Interval: {}, wait_for: {}", __func__, interval, wait_for);

        recording_interval = interval;
        wait_for_recorder = wait_for;
    }

    DataRecorder::~DataRecorder()
    {
        LOG_TRACE_L2(log, "[{}] init", __func__);
    }

    void DataRecorder::add_storage(SignalStorage *storage)
    {
        auto items = storage->variables.size();
        if (items > 0)
        {
            Tracker t;
            t.storage = storage;
            t.size = storage->mem_size;
            t.index = tracker_index;
            t.row_pos = row_size;
            trackers.push_back(t);

            row_size += storage->mem_size;

            LOG_TRACE_L1(log, "[{}] Adding tracker, storage: {}", __func__, storage->name);

            tracker_index++;
        }
    }

    void DataRecorder::reset_update_status(std::size_t row)
    {
        LOG_TRACE_L1(log, "[{}] Init", __func__);
        for (auto &t : trackers)
        {
            updated_tracker[row][t.index] = false;
        }
    }

    void DataRecorder::print_headers()
    {
        LOG_TRACE_L1(log, "[{}] Init", __func__);
        file << "time";
        for (const auto &tracker : trackers)
        {
            for (const auto &var : tracker.storage->variables)
            {
                file << ',' << var.name;
            }
        }
        file << '\n';
    }

    void DataRecorder::init()
    {
        LOG_TRACE_L1(log, "[{}] Init", __func__);
        auto allocation_size = row_size * rows;

        data = std::make_unique<std::byte[]>(allocation_size);
        LOG_TRACE_L1(log, "[{}] Completed allocation", __func__);

        const std::size_t cols = trackers.size();

        updated_tracker.clear();
        updated_tracker.reserve(rows);
        for (std::size_t i = 0; i < rows; ++i)
        {
            std::vector<std::atomic<bool>> row(cols); // default-construct atoms
            for (auto &cell : row)
            {
                cell.store(false);
            }
            updated_tracker.emplace_back(std::move(row));
        }

        print_headers();
    }

    void DataRecorder::start_recording()
    {
        LOG_INFO(log, "[{}] Starting recording", __func__);
        running = true;
        worker = std::make_unique<std::thread>([this]()
                                               { loop(); });
        usleep(100);
    }

    void DataRecorder::stop_recording()
    {
        LOG_INFO(log, "[{}] Stop recording", __func__);
        if (!running)
        {
            return;
        }

        running = false;

        usleep(100);
        update();

        if (worker && worker->joinable())
        {
            worker->join();
        }

        for (int i = 1; i <= rows; i++)
        {
            bool print = false;
            auto row = static_cast<uint16_t>((head + i) % rows);
            for (auto &tracker : trackers)
            {
                if (updated_tracker[row][tracker.index])
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

    std::byte *DataRecorder::get_data_pos(std::size_t row, std::size_t offset)
    {
        return data.get() + row * row_size + offset;
    }

    void DataRecorder::print_row(uint16_t row)
    {
        IF_LOG({
            LOG_TRACE_L1(log, "[{}] Row: {}", __func__, row);
        });

        auto time_value = utils::time::ns_to_s(row_time_map[row]);

        file << time_value;
        for (const auto &tracker : trackers)
        {
            for (auto& var : tracker.storage->variables)
            {
                IF_LOG({
                    LOG_TRACE_L2(log, "[{}] Printing tracker: {}, item:{}", __func__, tracker.storage->name, var.name);
                });

                auto pos = var.position;
                auto type = var.type;
                file << ", ";
                if (updated_tracker[row][tracker.index])
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

    void DataRecorder::update()
    {
        LOG_TRACE_L1(log, "[{}] Notifying recording to update", __func__);
    }

    void DataRecorder::loop()
    {
        LOG_DEBUG(log, "[{}] Starting recording thread", __func__);

        while (running)
        {
            IF_LOG({
                LOG_TRACE_L2(log, "[{}] Looking for new content to write to file", __func__);
            });

            for (auto &tracker : trackers)
            {
                IF_LOG({
                    LOG_TRACE_L3(log, "[{}] Evaluating storage {}", __func__, tracker.storage->to_string());
                });

                for (std::size_t area = 0; area < tracker.storage->areas; ++area)
                {
                    auto storage = tracker.storage;
                    if (storage->new_data_flags[area])
                    {
                        IF_LOG({
                            LOG_TRACE_L1(log, "[{}] Found new data; area: {}", __func__, area);
                        });

                        process_new_data(tracker, storage, area);
                        storage->new_data_flags[area] = false;
                    }
                }
            }
        }

        LOG_DEBUG(log, "[{}] Exiting recording thread", __func__);
    }

    void DataRecorder::process_new_data(ssp4sim::signal::Tracker &tracker, signal::SignalStorage *storage, std::size_t area)
    {
        auto ts = storage->get_time(area);

        if (!time_row_map.contains(ts))
        {
            IF_LOG({
                LOG_TRACE_L1(log, "[{}] New print time: {}, last_print_time {}", __func__, ts, last_print_time);
            });

            last_print_time += recording_interval;

            head = static_cast<uint16_t>((head + 1) % rows);
            if (new_item_counter >= rows)
            {
                IF_LOG({
                    LOG_TRACE_L1(log, "[{}] Row already in use, print and reset. {}", __func__, head);
                });

                print_row(head);
                reset_update_status(head);
            }

            new_item_counter++;

            row_time_map[head] = ts;
            time_row_map[ts] = head;
            IF_LOG({
                LOG_TRACE_L1(log, "[{}] New row [{}] with time [{}]", __func__, head, ts);
            });
        }

        if (time_row_map.contains(ts))
        {
            auto row = time_row_map[ts];
            IF_LOG({
                LOG_TRACE_L1(log, "[{}] Copying new data; row {}, size: {}", __func__, row, tracker.size);
            });

            std::memcpy(get_data_pos(row, tracker.row_pos), storage->locations[area][0], tracker.size);
            updated_tracker[row][tracker.index] = true;
        }
    }

    void DataRecorder::wait_until_done()
    {
        bool unprocessed_data;
        do
        {
            unprocessed_data = false;
            for (auto &tracker : trackers)
            {
                for (std::size_t area = 0; area < tracker.storage->areas; ++area)
                {
                    if (tracker.storage->new_data_flags[area])
                    {
                        unprocessed_data = true;
                    }
                }
            }
        } while (unprocessed_data);
    }

}
