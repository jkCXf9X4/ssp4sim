#pragma once

#include "ssp4cpp/utils/log.hpp"
#include "utils/time.hpp"

#include <shared_config.hpp>

#include "ssp4sim_definitions.hpp"

#include "signal/storage.hpp"
#include "signal/record_tracker.hpp"
#include "signal/mpsc_event_queue.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <thread>
#include <atomic>
#include <string>
#include <unordered_map>
#include <vector>

namespace ssp4sim::signal
{

    /*
    * Solution build upon that both input and outputs are stored in the same row, if not then the output file will be incomplete....

    * new solution is needed later....

    * @todo: Log everything that is between +-2 steps from the print time to ensure that no data is lost but still avoiding to log everything
    */

    class RecorderSink
    {
    public:
        virtual ~RecorderSink() = default;

        virtual void on_storage_added(const SignalStorage *storage) = 0;

        virtual void init() = 0;

        virtual void start()
        {
        }

        virtual void on_event(const NewDataEvent &event) = 0;

        virtual void stop() = 0;
    };

    class DataRecorder
    {
    public:
        ssp4cpp::utils::log::Logger *log = nullptr;

        std::unique_ptr<std::thread> worker;

        std::atomic<bool> running = false;
        BoundedMpscEventQueue<NewDataEvent> event_queue;
        std::atomic<std::size_t> event_signal = 0;

        std::vector<RecorderStorageBuffer> storage_buffers;
        std::unordered_map<SignalStorage *, std::size_t> storage_indexes;

        std::vector<std::unique_ptr<RecorderSink>> sinks;

        // config
        bool wait_for_recorder = false;

        DataRecorder(bool wait_for);

        DataRecorder(const DataRecorder &) = delete;
        DataRecorder &operator=(const DataRecorder &) = delete;

        ~DataRecorder();

        void add_storage(SignalStorage *storage);

        void add_sink(std::unique_ptr<RecorderSink> sink);

        void init();

        void start_recording();

        void stop_recording();

        void loop();

        static void new_event(void *context, NewDataEvent event);

        void enqueue_event(NewDataEvent new_event);

        void process_new_data(const NewDataEvent &new_event);

    };
}
