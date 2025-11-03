#pragma once

#include "cutecpp/log.hpp"
#include "utils/time.hpp"

#include "ssp4sim_definitions.hpp"

#include "data_storage.hpp"

#include <fstream>
#include <mutex>
#include <thread>
#include <atomic>
#include <string>
#include <vector>
#include <mutex>
#include <condition_variable>

namespace ssp4sim::utils
{
    struct Tracker
    {
        DataStorage *storage;

        std::size_t size = 0;
        std::size_t index = 0;
        std::size_t row_pos = 0;

        uint64_t timestamp = 0;
    };

    /*
    * Solution build upon that both input and outputs are stored in the same row, if not then the output file will be incomplete....

    * new solution is needed later....

    * @todo: Log everything that is between +-2 steps from the print time to ensure that no data is lost but still avoiding to log everything
    */

    class DataRecorder
    {
    public:
        Logger log = Logger("ssp4sim.utils.DataRecorder", LogLevel::info);

        std::ofstream file;
        std::unique_ptr<std::thread> worker;

        std::atomic<bool> running;
        std::mutex event_mutex;
        std::condition_variable event;

        std::vector<Tracker> trackers;
        std::size_t tracker_index = 0;

        std::uint16_t head = 0;
        std::size_t new_item_counter = 0;
        const std::size_t rows = 50;

        std::size_t row_size = 0;

        std::map<std::uint64_t, std::uint64_t> row_time_map;
        std::map<std::uint64_t, std::uint64_t> time_row_map;
        std::unique_ptr<std::byte[]> data;
        std::vector<std::vector<bool>> updated_tracker; // [row][tracker] bool to signify if the tracker is updated

        const uint64_t recording_interval = 0;
        uint64_t last_print_time = 0;
        size_t printed_rows = 0;

        DataRecorder(const std::string &filename);

        DataRecorder(const DataRecorder &) = delete;
        DataRecorder &operator=(const DataRecorder &) = delete;

        ~DataRecorder();

        void add_storage(DataStorage *storage);

        void reset_update_status(std::size_t row);

        void print_headers();

        void init();

        void start_recording();

        void stop_recording();

        std::byte *get_data_pos(std::size_t row, std::size_t offset);

        void print_row(uint16_t row);

        void update();

        void loop();

        void process_new_data(ssp4sim::utils::Tracker &tracker, utils::DataStorage *storage, std::size_t area);

        void wait_until_done();
    };
}
