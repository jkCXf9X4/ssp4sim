#pragma once

#include "cutecpp/log.hpp"
#include "utils/time.hpp"

#include "ssp4sim_definitions.hpp"

#include "signal/storage.hpp"
#include "signal/record_tracker.hpp"

#include <fstream>
#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <atomic>
#include <string>
#include <vector>
#include <condition_variable>

namespace ssp4sim::signal
{

    /*
    * Solution build upon that both input and outputs are stored in the same row, if not then the output file will be incomplete....

    * new solution is needed later....

    * @todo: Log everything that is between +-2 steps from the print time to ensure that no data is lost but still avoiding to log everything
    */

    class DataRecorder
    {
    public:
        Logger log = Logger("ssp4sim.record.DataRecorder", LogLevel::info);

        std::ofstream file;
        std::unique_ptr<std::thread> worker;

        std::atomic<bool> running;
        std::mutex event_mutex;
        std::condition_variable event;

        std::vector<Tracker> trackers;
        std::size_t tracker_index = 0;

        std::uint16_t head = 0;
        std::size_t new_item_counter = 0;
        const std::size_t rows = 500;

        std::size_t row_size = 0;

        std::map<std::uint64_t, std::uint64_t> row_time_map;
        std::map<std::uint64_t, std::uint64_t> time_row_map;
        std::unique_ptr<std::byte[]> data;
        std::vector<std::vector<std::atomic<bool>>> updated_tracker; // [row][tracker] bool to signify if the tracker is updated

        uint64_t last_print_time = 0;
        size_t printed_rows = 0;

        // config
        uint64_t recording_interval = 0;
        bool wait_for_recorder = false;


        DataRecorder(const std::string &filename, uint64_t interval, bool wait_for);

        DataRecorder(const DataRecorder &) = delete;
        DataRecorder &operator=(const DataRecorder &) = delete;

        ~DataRecorder();

        void add_storage(SignalStorage *storage);

        void reset_update_status(std::size_t row);

        void print_headers();

        void init();

        void start_recording();

        void stop_recording();

        std::byte *get_data_pos(std::size_t row, std::size_t offset);

        void print_row(uint16_t row);

        void update();

        void loop();

        void process_new_data(ssp4sim::signal::Tracker &tracker, signal::SignalStorage *storage, std::size_t area);

        void wait_until_done();
    };
}
