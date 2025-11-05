#include "utils/data_recorder.hpp"

#include "cutecpp/log.hpp"
#include "utils/data_storage.hpp"
#include "utils/time.hpp"

#include "config.hpp"

#include "FMI2_Enums_Ext.hpp"

#include <condition_variable>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <map>
#include <memory>
#include <thread>
#include <unistd.h>
#include <utility>
#include <vector>

namespace ssp4sim::utils
{

    DataRecorder::DataRecorder(const std::string &filename)
        : file(filename, std::ios::out)
    {
        log(ext_trace)("[{}] Constructor", __func__);
        log(debug)("[{}] Recording interval {}", __func__, recording_interval);
        log(debug)("[{}] File {}, open {}", __func__, filename, file.is_open());

        recording_interval = utils::time::s_to_ns(utils::Config::getOr("simulation.recording.interval", 1.0));
    }

    DataRecorder::~DataRecorder()
    {
        log(ext_trace)("[{}] init", __func__);
    }

    void DataRecorder::add_storage(DataStorage *storage)
    {
        if (storage->items > 0)
        {
            Tracker t;
            t.storage = storage;
            t.size = storage->pos;
            t.index = tracker_index;
            t.row_pos = row_size;
            trackers.push_back(t);

            row_size += storage->pos;

            log(trace)("[{}] Adding tracker, storage: {}", __func__, storage->name);

            tracker_index++;
        }
    }

    void DataRecorder::reset_update_status(std::size_t row)
    {
        log(ext_trace)("[{}] Init", __func__);
        for (auto &t : trackers)
        {
            updated_tracker[row][t.index] = false;
        }
    }

    void DataRecorder::print_headers()
    {
        log(trace)("[{}] Init", __func__);
        file << "time";
        for (const auto &tracker : trackers)
        {
            for (const auto &name : tracker.storage->names)
            {
                file << ',' << name;
            }
        }
        file << '\n';
    }

    void DataRecorder::init()
    {
        log(trace)("[{}] Init", __func__);
        auto allocation_size = row_size * rows;

        data = std::make_unique<std::byte[]>(allocation_size);
        log(trace)("[{}] Completed allocation", __func__);

        updated_tracker.reserve(rows);
        for (int r = 0; r < rows; r++)
        {
            std::vector<bool> trackers_status;
            trackers_status.reserve(trackers.size());
            for (auto &t : trackers)
            {
                trackers_status.push_back(false);
            }

            updated_tracker.push_back(std::move(trackers_status));
            reset_update_status(r);
        }
        print_headers();
    }

    void DataRecorder::start_recording()
    {
        log(info)("[{}] Starting recording", __func__);
        running = true;
        worker = std::make_unique<std::thread>([this]()
                                               { loop(); });
        usleep(100);
    }

    void DataRecorder::stop_recording()
    {
        log(info)("[{}] Stop recording", __func__);
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
            log(trace)("[{}] Row: {}", __func__, row);
        });

        auto time_value = utils::time::ns_to_s(row_time_map[row]);

        file << time_value;
        for (const auto &tracker : trackers)
        {
            for (int item = 0; item < tracker.storage->items; ++item)
            {
                IF_LOG({
                    log(ext_trace)("[{}] Printing tracker: {}, item:{}", __func__, tracker.storage->name, item);
                });

                auto pos = tracker.storage->positions[item];
                auto type = tracker.storage->types[item];
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
        log(ext_trace)("[{}] Notifying recording to update", __func__);
    }

    void DataRecorder::loop()
    {
        log(debug)("[{}] Starting recording thread", __func__);

        while (running)
        {
            IF_LOG({
                log(ext_trace)("[{}] Looking for new content to write to file", __func__);
            });

            for (auto &tracker : trackers)
            {
                IF_LOG({
                    log(ext_trace)("[{}] Evaluating storage {} {}", __func__, tracker.storage->to_string(), tracker.storage->index);
                });

                for (std::size_t area = 0; area < tracker.storage->areas; ++area)
                {
                    auto storage = tracker.storage;
                    if (storage->new_data_flags[area])
                    {
                        IF_LOG({
                            log(trace)("[{}] Found new data; area: {}", __func__, area);
                        });

                        process_new_data(tracker, storage, area);
                        storage->new_data_flags[area] = false;
                    }
                }
            }
        }

        log(debug)("[{}] Exiting recording thread", __func__);
    }

    void DataRecorder::process_new_data(ssp4sim::utils::Tracker &tracker, utils::DataStorage *storage, std::size_t area)
    {
        auto ts = storage->timestamps[area];

        if (!time_row_map.contains(ts))
        {
            IF_LOG({
                log(trace)("[{}] New print time: {}, last_print_time {}", __func__, ts, last_print_time);
            });

            last_print_time += recording_interval;

            head = static_cast<uint16_t>((head + 1) % rows);
            if (new_item_counter >= rows)
            {
                IF_LOG({
                    log(trace)("[{}] Row already in use, print and reset. {}", __func__, head);
                });

                print_row(head);
                reset_update_status(head);
            }

            new_item_counter++;

            row_time_map[head] = ts;
            time_row_map[ts] = head;
            IF_LOG({
                log(trace)("[{}] New row [{}] with time [{}]", __func__, head, ts);
            });
        }

        if (time_row_map.contains(ts))
        {
            auto row = time_row_map[ts];
            IF_LOG({
                log(trace)("[{}] Copying new data; row {}, size: {}", __func__, row, tracker.size);
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

