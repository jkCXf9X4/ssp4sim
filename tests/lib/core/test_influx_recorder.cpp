#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include <cpr/cpr.h>

#include "signal/influx_recorder_sink.hpp"
#include "signal/recorder.hpp"

#include "utils/time.hpp"
#include "ssp4sim_definitions.hpp"

#include <cstdlib>
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

using ssp4sim::signal::DataRecorder;
using ssp4sim::signal::InfluxRecorderSink;
using ssp4sim::signal::InfluxWriter;
using ssp4sim::signal::NewDataEvent;
using ssp4sim::signal::SignalStorage;
using ssp4sim::types::DataType;

namespace fs = std::filesystem;
namespace sim_time = ssp4sim::utils::time;

namespace
{
    struct RecordingInfluxWriter final : public InfluxWriter
    {
        std::vector<std::size_t> batch_sizes;
        std::vector<influxdb::Point> points;
        std::size_t flush_calls = 0;
        bool throw_on_write = false;
        bool throw_on_flush = false;

        void batch_of(std::size_t size) override
        {
            batch_sizes.push_back(size);
        }

        void write(influxdb::Point point) override
        {
            points.emplace_back(std::move(point));
            if (throw_on_write)
            {
                throw std::runtime_error("write failed");
            }
        }

        void flush_batch() override
        {
            ++flush_calls;
            if (throw_on_flush)
            {
                throw std::runtime_error("flush failed");
            }
        }
    };

    std::optional<std::string> tag_value(const influxdb::Point &point, std::string_view name)
    {
        const auto &tags = point.getTagSet();
        const auto it = std::find_if(tags.begin(), tags.end(), [name](const auto &tag)
                                     { return tag.first == name; });
        if (it == tags.end())
        {
            return std::nullopt;
        }
        return it->second;
    }

    template <typename T>
    std::optional<T> field_value(const influxdb::Point &point, std::string_view name)
    {
        const auto &fields = point.getFieldSet();
        const auto it = std::find_if(fields.begin(), fields.end(), [name](const auto &field)
                                     { return field.first == name; });
        if (it == fields.end())
        {
            return std::nullopt;
        }

        const auto *value = std::get_if<T>(&it->second);
        if (value == nullptr)
        {
            return std::nullopt;
        }

        return *value;
    }

    fs::path test_path(const std::string &name)
    {
        return fs::path(SSP4SIM_PROJECT_ROOT) / "build" / name;
    }

    void remove_if_exists(const fs::path &path)
    {
        if (fs::exists(path))
        {
            fs::remove(path);
        }
    }

    std::optional<std::string> discover_influx_token()
    {
        if (const char *env_token = std::getenv("SSP4SIM_INFLUX_TOKEN"); env_token != nullptr && *env_token != '\0')
        {
            return std::string(env_token);
        }

        const char *home = std::getenv("HOME");
        if (home == nullptr || *home == '\0')
        {
            return std::nullopt;
        }

        const auto config_path = fs::path(home) / ".influxdb" / "docker" / "explorer" / "config" / "config.json";
        if (!fs::exists(config_path))
        {
            return std::nullopt;
        }

        std::ifstream stream(config_path);
        if (!stream)
        {
            return std::nullopt;
        }

        const auto config = nlohmann::json::parse(stream, nullptr, false);
        if (config.is_discarded() || !config.is_object())
        {
            return std::nullopt;
        }

        const auto token = config.value("DEFAULT_API_TOKEN", "");
        if (token.empty())
        {
            return std::nullopt;
        }

        return token;
    }

    std::string influx_base_url()
    {
        if (const char *env_url = std::getenv("SSP4SIM_INFLUX_BASE_URL"); env_url != nullptr && *env_url != '\0')
        {
            return env_url;
        }

        return "http://localhost:8181";
    }

    ssp4sim::InfluxRecordingConfig make_influx_config(
        std::string url,
        std::string measurement,
        std::string run,
        std::size_t batch_size,
        std::string token = {},
        std::string db = "ssp4sim")
    {
        ssp4sim::InfluxRecordingConfig config;
        config.enable = true;
        config.url = std::move(url);
        config.db = std::move(db);
        config.token = std::move(token);
        config.measurement = std::move(measurement);
        config.run = std::move(run);
        config.batch_size = batch_size;
        return config;
    }

    cpr::Response query_influx_sql(const std::string &token, const std::string &sql)
    {
        const auto body = nlohmann::json{{"db", "ssp4sim"}, {"q", sql}}.dump();
        return cpr::Post(
            cpr::Url{influx_base_url() + "/api/v3/query_sql"},
            cpr::Header{{"Authorization", "Bearer " + token}, {"Content-Type", "application/json"}},
            cpr::Body{body},
            cpr::Timeout{5000},
            cpr::ConnectTimeout{1000});
    }
}

TEST_CASE("Influx recorder sink emits one point per signal", "[DataRecorder][Influx]")
{
    const auto csv_path = test_path("test_influx_recorder.csv");
    remove_if_exists(csv_path);

    auto writer = std::make_unique<RecordingInfluxWriter>();
    auto *writer_ptr = writer.get();

    const auto run_start = std::chrono::system_clock::time_point{std::chrono::seconds{100}};

    DataRecorder recorder(false);
    recorder.add_sink(std::make_unique<InfluxRecorderSink>(
        make_influx_config("http://unused", "ssp4sim_signal", "run-fixed", 7),
        run_start,
        std::move(writer)));

    SignalStorage storage(1, "source");
    storage.add("source.temperature", DataType::real, 1);
    storage.add("source.mode", DataType::integer, 1);
    storage.add("source.enabled", DataType::boolean, 1);
    storage.add("source.label", DataType::string, 1);
    storage.allocate();

    const auto type_real = storage.variables[0].type.to_string();
    const auto type_integer = storage.variables[1].type.to_string();
    const auto type_boolean = storage.variables[2].type.to_string();
    const auto type_string = storage.variables[3].type.to_string();

    recorder.add_storage(&storage);
    recorder.init();
    recorder.start_recording();

    const auto timestamp = 3ULL * sim_time::nanoseconds_per_second + 123ULL;
    const std::size_t area = storage.push(timestamp);
    const double temperature = 42.5;
    const int mode = -3;
    const bool enabled = true;
    const std::string label = "hello";

    std::memcpy(storage.get_item(area, 0), &temperature, sizeof(double));
    std::memcpy(storage.get_item(area, 1), &mode, sizeof(int));
    std::memcpy(storage.get_item(area, 2), &enabled, sizeof(bool));
    auto *label_ptr = reinterpret_cast<std::string *>(storage.get_item(area, 3));
    *label_ptr = label;

    storage.flag_new_data(area);

    recorder.stop_recording();

    REQUIRE(writer_ptr->batch_sizes == std::vector<std::size_t>{7});
    REQUIRE(writer_ptr->flush_calls == 1);
    REQUIRE(writer_ptr->points.size() == 4);

    const auto expected_timestamp = run_start + std::chrono::nanoseconds(timestamp);
    const auto expected_simulation_time_s = sim_time::ns_to_s(timestamp);

    REQUIRE(writer_ptr->points[0].getName() == "ssp4sim_signal");
    REQUIRE(tag_value(writer_ptr->points[0], "run").value() == "run-fixed");
    REQUIRE(tag_value(writer_ptr->points[0], "storage").value() == "source");
    REQUIRE(tag_value(writer_ptr->points[0], "signal").value() == "source.temperature");
    REQUIRE(tag_value(writer_ptr->points[0], "type").value() == type_real);
    REQUIRE(field_value<double>(writer_ptr->points[0], "value").value() == temperature);
    REQUIRE(field_value<double>(writer_ptr->points[0], "simulation_time_s").value() == expected_simulation_time_s);
    REQUIRE(writer_ptr->points[0].getTimestamp() == expected_timestamp);

    REQUIRE(writer_ptr->points[1].getName() == "ssp4sim_signal");
    REQUIRE(tag_value(writer_ptr->points[1], "signal").value() == "source.mode");
    REQUIRE(tag_value(writer_ptr->points[1], "type").value() == type_integer);
    REQUIRE(field_value<double>(writer_ptr->points[1], "value").value() == static_cast<double>(mode));
    REQUIRE(writer_ptr->points[1].getTimestamp() == expected_timestamp);

    REQUIRE(writer_ptr->points[2].getName() == "ssp4sim_signal");
    REQUIRE(tag_value(writer_ptr->points[2], "signal").value() == "source.enabled");
    REQUIRE(tag_value(writer_ptr->points[2], "type").value() == type_boolean);
    REQUIRE(field_value<double>(writer_ptr->points[2], "value").value() == (enabled ? 1.0 : 0.0));
    REQUIRE(writer_ptr->points[2].getTimestamp() == expected_timestamp);

    REQUIRE(writer_ptr->points[3].getName() == "ssp4sim_signal");
    REQUIRE(tag_value(writer_ptr->points[3], "signal").value() == "source.label");
    REQUIRE(tag_value(writer_ptr->points[3], "type").value() == type_string);
    REQUIRE(field_value<std::string>(writer_ptr->points[3], "value_string").value() == label);
    REQUIRE(writer_ptr->points[3].getTimestamp() == expected_timestamp);

    remove_if_exists(csv_path);
}

TEST_CASE("Influx recorder sink tolerates writer creation failure", "[Influx]")
{
    auto csv_path = test_path("test_influx_recorder_failure.csv");
    remove_if_exists(csv_path);

    InfluxRecorderSink sink(make_influx_config("bad://invalid-url", "ssp4sim_signal", "run-fixed", 1));

    SignalStorage storage(1, "source");
    storage.add("source.temperature", DataType::real, 1);
    storage.allocate();

    sink.on_storage_added(&storage);

    REQUIRE_NOTHROW(sink.init());

    const auto timestamp = sim_time::nanoseconds_per_second;
    const std::size_t area = storage.push(timestamp);
    const double temperature = 7.5;
    std::memcpy(storage.get_item(area, 0), &temperature, sizeof(double));

    NewDataEvent event;
    event.storage = &storage;
    event.area = area;
    event.timestamp = timestamp;
    event.buffer = storage.get_item(area, 0);
    event.recorder_storage_index = 0;

    REQUIRE_NOTHROW(sink.on_event(event));
    REQUIRE_NOTHROW(sink.stop());

    remove_if_exists(csv_path);
}

TEST_CASE("Influx recorder sink disables itself after write failures", "[Influx]")
{
    auto writer = std::make_unique<RecordingInfluxWriter>();
    auto *writer_ptr = writer.get();
    writer_ptr->throw_on_write = true;

    InfluxRecorderSink sink(
        make_influx_config("http://unused", "ssp4sim_signal", "run-fixed", 3),
        std::chrono::system_clock::time_point{std::chrono::seconds{5}},
        std::move(writer));

    SignalStorage storage(1, "source");
    storage.add("source.temperature", DataType::real, 1);
    storage.allocate();

    sink.on_storage_added(&storage);
    sink.init();
    sink.start();

    const auto timestamp = 2ULL * sim_time::nanoseconds_per_second;
    const std::size_t area = storage.push(timestamp);
    const double temperature = 8.25;
    std::memcpy(storage.get_item(area, 0), &temperature, sizeof(double));

    NewDataEvent event;
    event.storage = &storage;
    event.area = area;
    event.timestamp = timestamp;
    event.buffer = storage.get_item(area, 0);
    event.recorder_storage_index = 0;

    REQUIRE_NOTHROW(sink.on_event(event));
    REQUIRE_NOTHROW(sink.on_event(event));
    REQUIRE(writer_ptr->points.size() == 1);

    REQUIRE_NOTHROW(sink.stop());
    REQUIRE(writer_ptr->flush_calls == 1);
}

TEST_CASE("Influx recorder sink writes to a live InfluxDB instance", "[Influx][integration]")
{
    const auto token = discover_influx_token();
    if (!token)
    {
        SKIP("Set SSP4SIM_INFLUX_TOKEN or provide $HOME/.influxdb/docker/explorer/config/config.json");
    }

    const auto csv_path = test_path("test_influx_recorder_live.csv");
    remove_if_exists(csv_path);

    const auto measurement = std::string("ssp4sim_live_influx");
    const auto run_name = std::string("run-live-") + std::to_string(sim_time::time_now_ns());
    const auto base_url = influx_base_url();
    const auto run_start = std::chrono::system_clock::time_point{std::chrono::seconds{1}};

    DataRecorder recorder(false);
    recorder.add_sink(std::make_unique<InfluxRecorderSink>(
        make_influx_config(base_url, measurement, run_name, 1, *token),
        run_start));

    SignalStorage storage(1, "live_source");
    storage.add("live_source.temperature", DataType::real, 1);
    storage.allocate();

    const auto type_real = storage.variables[0].type.to_string();

    recorder.add_storage(&storage);
    recorder.init();
    recorder.start_recording();

    const auto timestamp = 234567890ULL;
    const std::size_t area = storage.push(timestamp);
    const double temperature = 19.75;
    std::memcpy(storage.get_item(area, 0), &temperature, sizeof(double));

    storage.flag_new_data(area);
    recorder.stop_recording();

    const auto expected_time = std::string("1970-01-01T00:00:01.234567890");
    const auto expected_simulation_time_s = sim_time::ns_to_s(timestamp);
    const auto sql = std::string("SELECT run, storage, signal, type, value, simulation_time_s, time FROM ") + measurement + " WHERE run = '" + run_name + "' LIMIT 1";

    std::optional<nlohmann::json> row;
    const auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds{10};
    while (std::chrono::steady_clock::now() < deadline)
    {
        const auto response = query_influx_sql(*token, sql);
        if (response.status_code == 0)
        {
            SKIP("InfluxDB is not reachable at " + influx_base_url());
        }

        REQUIRE(response.status_code == 200);

        const auto rows = nlohmann::json::parse(response.text, nullptr, false);
        REQUIRE_FALSE(rows.is_discarded());
        REQUIRE(rows.is_array());

        if (!rows.empty())
        {
            row = rows.front();
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds{100});
    }

    REQUIRE(row.has_value());
    REQUIRE(row->at("run").get<std::string>() == run_name);
    REQUIRE(row->at("storage").get<std::string>() == "live_source");
    REQUIRE(row->at("signal").get<std::string>() == "live_source.temperature");
    REQUIRE(row->at("type").get<std::string>() == type_real);
    REQUIRE(row->at("value").get<double>() == Catch::Approx(temperature));
    REQUIRE(row->at("simulation_time_s").get<double>() == Catch::Approx(expected_simulation_time_s));
    REQUIRE(row->at("time").get<std::string>() == expected_time);

    remove_if_exists(csv_path);
}
