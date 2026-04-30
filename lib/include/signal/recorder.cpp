#include "signal/recorder.hpp"

#include "signal/csv_recorder_sink.hpp"
#include "signal/storage.hpp"

#include <memory>
#include <thread>
#include <utility>

namespace ssp4sim::signal
{

    DataRecorder::DataRecorder(const std::string &filename, uint64_t interval, bool wait_for)
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.signal.DataRecorder"))
    {
        LOG_TRACE_L2(log, "[{func}] Constructor", __func__);

        recording_interval = interval;
        wait_for_recorder = wait_for;

        add_sink(std::make_unique<CsvRecorderSink>(filename, recording_interval));

        LOG_DEBUG(log, "[{func}] Interval: {interval}, wait_for: {wait_for}", __func__, recording_interval, wait_for_recorder);
    }

    DataRecorder::~DataRecorder()
    {
        LOG_TRACE_L2(log, "[{func}] init", __func__);
        if (running)
        {
            stop_recording();
        }
    }

    void DataRecorder::add_sink(std::unique_ptr<RecorderSink> sink)
    {
        sinks.emplace_back(std::move(sink));
    }

    void DataRecorder::add_storage(SignalStorage *storage)
    {
        auto items = storage->variables.size();
        if (items > 0)
        {
            const auto buffer_capacity = 50;

            if (storage_indexes.contains(storage))
            {
                LOG_WARNING(log, "[{}] Storage already registered: {storage}", __func__, storage->name);
                return;
            }

            const auto recorder_index = storage_buffers.size();
            storage_buffers.emplace_back(RecorderStorageBuffer(storage, recorder_index, buffer_capacity));
            storage_indexes[storage] = recorder_index;

            storage->register_callback(&DataRecorder::new_event, this);

            for (auto &sink : sinks)
            {
                sink->on_storage_added(storage);
            }

            LOG_TRACE_L1(log, "[{func}] Adding RecorderStorageBuffer, storage: {storage}", __func__, storage->name);
        }
    }

    void DataRecorder::init()
    {
        LOG_TRACE_L1(log, "[{func}] Init", __func__);
        for (auto &sink : sinks)
        {
            sink->init();
        }
    }

    void DataRecorder::start_recording()
    {
        LOG_INFO(log, "[{func}] Starting recording", __func__);
        running = true;
        worker = std::make_unique<std::thread>([this]()
                                               { loop(); });
    }

    void DataRecorder::stop_recording()
    {
        LOG_INFO(log, "[{func}] Stop recording", __func__);
        if (!running)
        {
            return;
        }

        running = false;
        event.notify_all();

        if (worker && worker->joinable())
        {
            worker->join();
        }

        for (auto &sink : sinks)
        {
            sink->stop();
        }
    }

    // callback from storage
    void DataRecorder::new_event(void *context, NewDataEvent new_event)
    {
        auto recorder = static_cast<DataRecorder *>(context);
        if (recorder)
        {
            recorder->enqueue_event(new_event);
        }
    }

    void DataRecorder::enqueue_event(NewDataEvent new_event)
    {
        auto storage_index = storage_indexes.find(new_event.storage);
        if (storage_index == storage_indexes.end())
        {
            LOG_WARNING(log, "[{func}] Ignoring event for unregistered storage {}", __func__, new_event.storage->name);
            return;
        }

        auto &recorder_storage = this->storage_buffers[storage_index->second];
        new_event.recorder_storage_index = storage_index->second;

        std::size_t target_area;

        // TODO: add wait_for_recorder here if full
        if (recorder_storage.try_push(new_event.area, target_area))
        {
            new_event.buffer = recorder_storage.buffers->get_item(target_area);
            {
                std::lock_guard<std::mutex> lock(event_mutex);
                event_queue.push_back(new_event);
                event.notify_one();
            }
        }
        else
        {
            LOG_WARNING_LIMIT_EVERY_N(100000, log, "[{func}] Storage: {} full for area {}", __func__, recorder_storage.storage->name, new_event.area);
        }
    }

    void DataRecorder::loop()
    {
        LOG_DEBUG(log, "[{func}] Starting recording thread", __func__);

        while (true)
        {
            NewDataEvent new_event;
            {
                std::unique_lock<std::mutex> lock(event_mutex);
                event.wait(lock, [this]()
                           { return !running || !event_queue.empty(); });

                if (event_queue.empty())
                {
                    if (!running)
                    {
                        break;
                    }
                    continue;
                }

                new_event = event_queue.front();
                event_queue.pop_front();
            }

            process_new_data(new_event);

            if (new_event.recorder_storage_index >= storage_buffers.size())
            {
                LOG_WARNING(log, "[{func}] Invalid recorder buffer index {} for storage {}", __func__, new_event.recorder_storage_index, new_event.storage->name);
                continue;
            }
            auto &recorder_storage = storage_buffers[new_event.recorder_storage_index];
            recorder_storage.pop();
        }

        LOG_DEBUG(log, "[{func}] Exiting recording thread", __func__);
    }

    void DataRecorder::process_new_data(const NewDataEvent &new_event)
    {
        for (auto &sink : sinks)
        {
            sink->on_event(new_event);
        }

    }
}
