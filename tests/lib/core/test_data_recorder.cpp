#include <catch.hpp>
#include "ssp4cpp/utils/log.hpp"

#include "simulation/signal/sinks/csv_recorder_sink.hpp"
#include "utils/time/time.hpp"
#include "utils/memory/allocator.hpp"
#include "simulation/signal/recorder.hpp"

#include "ssp4sim_definitions.hpp"

#include <catch2/matchers/catch_matchers_floating_point.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <filesystem>
#include <string>
#include <thread>
#include <chrono>
#include <vector>

using ssp4sim::signal::DataRecorder;
using ssp4sim::signal::CsvRecorderSink;
using ssp4sim::signal::NewDataEvent;
using ssp4sim::signal::RecorderSink;
using ssp4sim::signal::SignalStorage;
using ssp4sim::types::DataType;

namespace sim_time = ssp4sim::utils::time;
namespace fs = std::filesystem;

const fs::path project_root{SSP4SIM_PROJECT_ROOT};

// Helper function to check if file exists and contains expected data
bool check_file_contains(const std::string &filename, const std::string &expected)
{
    std::ifstream file(filename);
    if (!file.is_open())
        return false;

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    return content.find(expected) != std::string::npos;
}

void remove_if_existing(std::string name)
{
    // Remove any existing test file
    if (fs::exists(name))
    {
        fs::remove(name);
    }
}

class CollectingSink : public RecorderSink
{
public:
    std::vector<NewDataEvent> events;
    std::vector<std::string> storage_names;
    std::vector<double> first_values;

    void on_storage_added(const SignalStorage *storage) override
    {
        storage_names.push_back(storage->name);
    }

    void init() override
    {
    }

    void on_event(const NewDataEvent &event) override
    {
        events.push_back(event);
        storage_names.push_back(event.storage->name);
        if (event.buffer && event.storage->area_byte_size >= sizeof(double))
        {
            double value = 0.0;
            std::memcpy(&value, event.buffer, sizeof(double));
            first_values.push_back(value);
        }
    }

    void stop() override
    {
    }
};

// ---------------------------------------------------------------------------
// Description: Verifies CSV file creation on construction and persistence
//              after destruction
// Rationale:   File lifecycle — recorder must create files and not delete them
// ---------------------------------------------------------------------------
TEST_CASE("DataRecorder initialization and cleanup", "[DataRecorder]")
{
    // Use a temporary filename for testing
    const fs::path test_filename = project_root / "build" / "test_recorder_output.csv";
    remove_if_existing(test_filename);

    // Scope for the DataRecorder to ensure it's destructed properly
    {
        DataRecorder recorder(false);
        recorder.add_sink(std::make_unique<CsvRecorderSink>(test_filename, 1000));
        // Check if file was created
        REQUIRE(fs::exists(test_filename));
    }

    // After destruction, file should still exist
    REQUIRE(fs::exists(test_filename));

    // Clean up the test file
    fs::remove(test_filename);
}

// ---------------------------------------------------------------------------
// Description: Verifies storage registration count and CSV header line
// Rationale:   CSV header must match registered signal names
// ---------------------------------------------------------------------------
TEST_CASE("DataRecorder configures trackers and headers", "[DataRecorder]")
{
    const fs::path test_filename = project_root / "build" / "test_recorder_headers.csv";
    remove_if_existing(test_filename);

    DataRecorder recorder(false);
    recorder.add_sink(std::make_unique<CsvRecorderSink>(test_filename, 1000));

    SignalStorage storage(2, "signals");
    storage.add_variable("signals.real", DataType::real, 1);
    storage.add_variable("signals.int", DataType::integer, 1);
    storage.allocate();

    recorder.add_storage(&storage);

    auto registered_buffers = std::count_if(recorder.storage_buffers.begin(), recorder.storage_buffers.end(), [](const auto &buffer)
                                            { return buffer.storage != nullptr; });
    REQUIRE(registered_buffers == 1);

    recorder.init();

    REQUIRE(check_file_contains(test_filename.string(), "time,signals.real,signals.int"));

    fs::remove(test_filename);
}

// ---------------------------------------------------------------------------
// Description: Verifies CSV contains expected values for real and int signals
// Rationale:   Core recording contract — data in storage appears in CSV output
// ---------------------------------------------------------------------------
TEST_CASE("DataRecorder writes new rows when storages provide data", "[DataRecorder]")
{
    const fs::path test_filename = project_root / "build" / "test_recorder_rows.csv";
    remove_if_existing(test_filename);

    DataRecorder recorder(false);
    recorder.add_sink(std::make_unique<CsvRecorderSink>(test_filename, 1000));

    SignalStorage storage(2, "signals");
    storage.add_variable("signals.temperature", DataType::real, 1);
    storage.add_variable("signals.mode", DataType::integer, 0);
    storage.allocate();

    recorder.add_storage(&storage);
    recorder.init();
    recorder.start_recording();

    const uint64_t timestamp = 1ULL * sim_time::nanoseconds_per_second;
    const std::size_t area = storage.push(timestamp);
    const double temperature = 42.5;
    const int mode = 7;

    std::memcpy(storage.get_item(area, 0), &temperature, sizeof(double));
    std::memcpy(storage.get_item(area, 1), &mode, sizeof(int));

    storage.flag_new_data(area);

    recorder.stop_recording();

    REQUIRE(check_file_contains(test_filename.string(), "1, 42.5"));
    REQUIRE(check_file_contains(test_filename.string(), ", 7"));

    fs::remove(test_filename);
}

// ---------------------------------------------------------------------------
// Description: Verifies multi-storage data appears in single CSV row
// Rationale:   Coalescing required for time-aligned output
// ---------------------------------------------------------------------------
TEST_CASE("DataRecorder coalesces updates from multiple storages", "[DataRecorder]")
{
    const fs::path test_filename = project_root / "build" / "test_recorder_multistorage.csv";
    remove_if_existing(test_filename);

    DataRecorder recorder(false);
    recorder.add_sink(std::make_unique<CsvRecorderSink>(test_filename, 1000));

    SignalStorage primary(2, "primary");
    primary.add_variable("primary.temperature", DataType::real, 1);
    primary.add_variable("primary.mode", DataType::integer, 0);
    primary.allocate();

    SignalStorage secondary(2, "secondary");
    secondary.add_variable("secondary.pressure", DataType::real, 1);
    secondary.add_variable("secondary.index", DataType::integer, 2);
    secondary.allocate();

    recorder.add_storage(&primary);
    recorder.add_storage(&secondary);
    recorder.init();
    recorder.start_recording();

    constexpr uint64_t timestamp = 1ULL * sim_time::nanoseconds_per_second;
    const std::size_t area = primary.push(timestamp);
    secondary.push(timestamp);
    const double primary_temp = 42.5;
    const int primary_mode = 7;
    const double secondary_pressure = 3.14;
    const int secondary_index = -2;

    std::memcpy(primary.get_item(area, 0), &primary_temp, sizeof(double));
    std::memcpy(primary.get_item(area, 1), &primary_mode, sizeof(int));
    std::memcpy(secondary.get_item(area, 0), &secondary_pressure, sizeof(double));
    std::memcpy(secondary.get_item(area, 1), &secondary_index, sizeof(int));

    primary.flag_new_data(area);
    secondary.flag_new_data(area);

    recorder.stop_recording();

    std::ifstream file(test_filename);
    REQUIRE(file.is_open());

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line))
    {
        lines.push_back(line);
    }

    auto data_line = std::find_if(lines.begin(), lines.end(), [](const std::string &l)
                                  { return l.starts_with("1"); });
    REQUIRE(data_line != lines.end());
    REQUIRE(data_line->find("42.5") != std::string::npos);
    REQUIRE(data_line->find(", 7") != std::string::npos);
    REQUIRE(data_line->find("3.14") != std::string::npos);
    REQUIRE(data_line->find("-2") != std::string::npos);

    const auto occurrences_of_timestamp = std::count_if(lines.begin(), lines.end(), [](const std::string &l)
                                                        { return l.starts_with("1"); });
    REQUIRE(occurrences_of_timestamp == 1);

    fs::remove(test_filename);
}

// ---------------------------------------------------------------------------
// Description: Verifies interval-based recording skips intra-interval data
// Rationale:   Interval recording is a performance optimization
// ---------------------------------------------------------------------------
TEST_CASE("DataRecorder respects CSV recording interval", "[DataRecorder]")
{
    const fs::path test_filename = project_root / "build" / "test_recorder_interval.csv";
    remove_if_existing(test_filename);

    DataRecorder recorder(false);
    recorder.add_sink(std::make_unique<CsvRecorderSink>(test_filename, sim_time::nanoseconds_per_second));

    SignalStorage primary(1, "primary");
    primary.add_variable("primary.temperature", DataType::real, 1);
    primary.allocate();

    SignalStorage secondary(1, "secondary");
    secondary.add_variable("secondary.pressure", DataType::real, 1);
    secondary.allocate();

    recorder.add_storage(&primary);
    recorder.add_storage(&secondary);
    recorder.init();
    recorder.start_recording();

    const uint64_t first_timestamp = 1ULL * sim_time::nanoseconds_per_second;
    const std::size_t primary_area = primary.push(first_timestamp);
    const std::size_t secondary_area = secondary.push(first_timestamp);
    const double primary_temp = 42.5;
    const double secondary_pressure = 3.14;

    std::memcpy(primary.get_item(primary_area, 0), &primary_temp, sizeof(double));
    std::memcpy(secondary.get_item(secondary_area, 0), &secondary_pressure, sizeof(double));

    primary.flag_new_data(primary_area);
    secondary.flag_new_data(secondary_area);

    const uint64_t skipped_timestamp = first_timestamp + sim_time::nanoseconds_per_second / 2;
    const std::size_t skipped_area = primary.push(skipped_timestamp);
    const double skipped_temp = 9.9;
    std::memcpy(primary.get_item(skipped_area, 0), &skipped_temp, sizeof(double));
    primary.flag_new_data(skipped_area);

    const uint64_t next_timestamp = first_timestamp + sim_time::nanoseconds_per_second;
    const std::size_t next_area = secondary.push(next_timestamp);
    const double next_pressure = 7.7;
    std::memcpy(secondary.get_item(next_area, 0), &next_pressure, sizeof(double));
    secondary.flag_new_data(next_area);

    recorder.stop_recording();

    std::ifstream file(test_filename);
    REQUIRE(file.is_open());

    std::vector<std::string> lines;
    std::string line;
    while (std::getline(file, line))
    {
        lines.push_back(line);
    }

    REQUIRE(lines.size() == 3);
    REQUIRE(lines[1].find("42.5") != std::string::npos);
    REQUIRE(lines[1].find("3.14") != std::string::npos);
    REQUIRE(lines[1].find("9.9") == std::string::npos);
    REQUIRE(lines[2].find("7.7") != std::string::npos);
    REQUIRE(lines[2].find("9.9") == std::string::npos);

    fs::remove(test_filename);
}

// ---------------------------------------------------------------------------
// Description: Verifies CollectingSink receives correct events with aligned
//              buffers, correct pointers, timestamps, indices
// Rationale:   Sink event dispatch is the extension point for custom backends
// Creep flag:  Alignment checks are buffer-allocation implementation details
// ---------------------------------------------------------------------------
TEST_CASE("DataRecorder dispatches raw events to registered sinks", "[DataRecorder]")
{
    const fs::path test_filename = project_root / "build" / "test_recorder_sink_events.csv";
    remove_if_existing(test_filename);

    DataRecorder recorder(false);
    recorder.add_sink(std::make_unique<CsvRecorderSink>(test_filename, 1000));
    auto sink = std::make_unique<CollectingSink>();
    auto *sink_ptr = sink.get();
    recorder.add_sink(std::move(sink));

    SignalStorage primary(2, "primary");
    primary.add_variable("primary.temperature", DataType::real, 1);
    primary.allocate();

    SignalStorage secondary(2, "secondary");
    secondary.add_variable("secondary.pressure", DataType::real, 1);
    secondary.allocate();

    recorder.add_storage(&primary);
    recorder.add_storage(&secondary);
    recorder.init();
    recorder.start_recording();

    constexpr uint64_t timestamp = 2ULL * sim_time::nanoseconds_per_second;
    const std::size_t primary_area = primary.push(timestamp);
    const std::size_t secondary_area = secondary.push(timestamp);
    const double primary_temperature = 4.2;
    const double secondary_pressure = 9.9;

    std::memcpy(primary.get_item(primary_area, 0), &primary_temperature, sizeof(double));
    std::memcpy(secondary.get_item(secondary_area, 0), &secondary_pressure, sizeof(double));

    primary.flag_new_data(primary_area);
    secondary.flag_new_data(secondary_area);

    recorder.stop_recording();

    REQUIRE(sink_ptr->events.size() == 2);
    REQUIRE(sink_ptr->events[0].storage == &primary);
    REQUIRE(sink_ptr->events[0].area == primary_area);
    REQUIRE(sink_ptr->events[0].timestamp == timestamp);
    REQUIRE(sink_ptr->events[0].recorder_storage_index == 0);
    REQUIRE(sink_ptr->events[0].buffer != nullptr);
    CHECK(reinterpret_cast<std::uintptr_t>(sink_ptr->events[0].buffer) % ssp4sim::utils::target_alignment == 0);
    REQUIRE(sink_ptr->events[1].storage == &secondary);
    REQUIRE(sink_ptr->events[1].area == secondary_area);
    REQUIRE(sink_ptr->events[1].timestamp == timestamp);
    REQUIRE(sink_ptr->events[1].recorder_storage_index == 1);
    REQUIRE(sink_ptr->events[1].buffer != nullptr);
    CHECK(reinterpret_cast<std::uintptr_t>(sink_ptr->events[1].buffer) % ssp4sim::utils::target_alignment == 0);
    REQUIRE(sink_ptr->first_values.size() == 2);
    REQUIRE(sink_ptr->first_values[0] == primary_temperature);
    REQUIRE(sink_ptr->first_values[1] == secondary_pressure);
    REQUIRE(std::find(sink_ptr->storage_names.begin(), sink_ptr->storage_names.end(), "primary") != sink_ptr->storage_names.end());
    REQUIRE(std::find(sink_ptr->storage_names.begin(), sink_ptr->storage_names.end(), "secondary") != sink_ptr->storage_names.end());

    fs::remove(test_filename);
}

// ---------------------------------------------------------------------------
// Description: Verifies events pushed before start_recording() are dispatched
//              after start_recording()
// Rationale:   Event buffering required because storages may produce data
//              before recorder is ready
// ---------------------------------------------------------------------------
TEST_CASE("DataRecorder buffers raw events before storage areas are overwritten", "[DataRecorder]")
{
    const fs::path test_filename = project_root / "build" / "test_recorder_stale_events.csv";
    remove_if_existing(test_filename);

    DataRecorder recorder(false);
    recorder.add_sink(std::make_unique<CsvRecorderSink>(test_filename, 1000));
    auto sink = std::make_unique<CollectingSink>();
    auto *sink_ptr = sink.get();
    recorder.add_sink(std::move(sink));

    SignalStorage storage(1, "signals");
    storage.add_variable("signals.temperature", DataType::real, 1);
    storage.allocate();

    recorder.add_storage(&storage);
    recorder.init();

    constexpr uint64_t stale_timestamp = 1ULL * sim_time::nanoseconds_per_second;
    constexpr uint64_t latest_timestamp = 2ULL * sim_time::nanoseconds_per_second;

    const std::size_t stale_area = storage.push(stale_timestamp);
    const double stale_temperature = 1.5;
    std::memcpy(storage.get_item(stale_area, 0), &stale_temperature, sizeof(double));
    storage.flag_new_data(stale_area);

    const std::size_t latest_area = storage.push(latest_timestamp);
    const double latest_temperature = 7.5;
    std::memcpy(storage.get_item(latest_area, 0), &latest_temperature, sizeof(double));
    storage.flag_new_data(latest_area);

    recorder.start_recording();
    recorder.stop_recording();

    REQUIRE(sink_ptr->events.size() == 2);
    REQUIRE(sink_ptr->events[0].storage == &storage);
    REQUIRE(sink_ptr->events[0].area == stale_area);
    REQUIRE(sink_ptr->events[0].timestamp == stale_timestamp);
    REQUIRE(sink_ptr->events[0].recorder_storage_index == 0);
    REQUIRE(sink_ptr->events[0].buffer != nullptr);
    CHECK(reinterpret_cast<std::uintptr_t>(sink_ptr->events[0].buffer) % ssp4sim::utils::target_alignment == 0);
    REQUIRE(sink_ptr->events[1].storage == &storage);
    REQUIRE(sink_ptr->events[1].area == latest_area);
    REQUIRE(sink_ptr->events[1].timestamp == latest_timestamp);
    REQUIRE(sink_ptr->events[1].recorder_storage_index == 0);
    REQUIRE(sink_ptr->events[1].buffer != nullptr);
    CHECK(reinterpret_cast<std::uintptr_t>(sink_ptr->events[1].buffer) % ssp4sim::utils::target_alignment == 0);
    REQUIRE(sink_ptr->first_values.size() == 2);
    REQUIRE(sink_ptr->first_values[0] == stale_temperature);
    REQUIRE(sink_ptr->first_values[1] == latest_temperature);

    fs::remove(test_filename);
}
