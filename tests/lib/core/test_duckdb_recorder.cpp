#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "signal/sinks/duckdb_recorder_sink.hpp"

#include "utils/time.hpp"

#include <duckdb.h>

#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>
#include <string_view>

namespace fs = std::filesystem;
namespace sim_time = ssp4sim::utils::time;

using ssp4sim::signal::DuckDbRecorderSink;
using ssp4sim::signal::NewDataEvent;
using ssp4sim::signal::SignalStorage;
using ssp4sim::types::DataType;

namespace
{
    fs::path test_path(const std::string &name)
    {
        return fs::path(SSP4SIM_PROJECT_ROOT) / "build" / name;
    }

    std::string sanitize_component(std::string_view value)
    {
        std::string sanitized;
        sanitized.reserve(value.size());
        for (const auto ch : value)
        {
            if (std::isalnum(static_cast<unsigned char>(ch)) != 0)
            {
                sanitized.push_back(ch);
            }
            else
            {
                sanitized.push_back('_');
            }
        }

        if (sanitized.empty())
        {
            return "default";
        }

        return sanitized;
    }

    std::string table_name_for(std::size_t index, std::string_view model, std::string_view storage_name)
    {
        std::string table_name = "duckdb_";
        table_name += std::to_string(index);
        if (!model.empty())
        {
            table_name.push_back('_');
            table_name += sanitize_component(model);
        }
        if (!storage_name.empty())
        {
            table_name.push_back('_');
            table_name += sanitize_component(storage_name);
        }

        return table_name;
    }

    std::string quote_identifier(const std::string &name)
    {
        std::string quoted = "\"";
        for (const auto ch : name)
        {
            if (ch == '"')
            {
                quoted += "\"\"";
            }
            else
            {
                quoted.push_back(ch);
            }
        }
        quoted.push_back('"');
        return quoted;
    }

    void remove_if_exists(const fs::path &path)
    {
        if (fs::exists(path))
        {
            fs::remove(path);
        }
    }

    void require_query(duckdb_connection connection, const std::string &query, duckdb_result &result)
    {
        REQUIRE(duckdb_query(connection, query.c_str(), &result) == DuckDBSuccess);
    }
}

TEST_CASE("DuckDB recorder sink writes per-storage tables", "[DataRecorder][DuckDB]")
{
    const auto db_path = test_path("test_duckdb_recorder.duckdb");
    remove_if_exists(db_path);

    DuckDbRecorderSink sink(db_path);

    SignalStorage storage(1, "Consumer.output");
    storage.add("Consumer.CPUtime", DataType::real, 1);
    storage.add("Consumer.EventCounter", DataType::integer, 1);
    storage.add("Consumer.enabled", DataType::boolean, 1);
    storage.add("Consumer.label", DataType::string, 1);
    storage.allocate();

    SignalStorage aux_storage(2, "Aux.output");
    aux_storage.add("Aux.value", DataType::real, 1);
    aux_storage.allocate();

    sink.on_storage_added(&storage);
    sink.on_storage_added(&aux_storage);
    sink.init();
    sink.start();

    const auto timestamp = 3ULL * sim_time::nanoseconds_per_second + 123ULL;
    const std::size_t area = storage.push(timestamp);
    const std::size_t aux_area = aux_storage.push(timestamp);
    const double cpu_time = 0.045515;
    const int event_counter = 16;
    const int enabled = 1;
    const std::string label = "hello";
    const double aux_value = 9.5;

    std::memcpy(storage.get_item(area, 0), &cpu_time, sizeof(double));
    std::memcpy(storage.get_item(area, 1), &event_counter, sizeof(int));
    std::memcpy(storage.get_item(area, 2), &enabled, sizeof(int));
    auto *label_ptr = reinterpret_cast<std::string *>(storage.get_item(area, 3));
    *label_ptr = label;
    std::memcpy(aux_storage.get_item(aux_area, 0), &aux_value, sizeof(double));

    NewDataEvent event;
    event.storage = &storage;
    event.area = area;
    event.timestamp = timestamp;
    event.buffer = storage.get_item(area, 0);
    event.recorder_storage_index = 0;

    NewDataEvent aux_event;
    aux_event.storage = &aux_storage;
    aux_event.area = aux_area;
    aux_event.timestamp = timestamp;
    aux_event.buffer = aux_storage.get_item(aux_area, 0);
    aux_event.recorder_storage_index = 1;

    REQUIRE_NOTHROW(sink.on_event(event));
    REQUIRE_NOTHROW(sink.on_event(aux_event));
    REQUIRE_NOTHROW(sink.stop());

    REQUIRE(fs::exists(db_path));

    duckdb_database database = nullptr;
    duckdb_connection connection = nullptr;
    REQUIRE(duckdb_open(db_path.string().c_str(), &database) == DuckDBSuccess);
    REQUIRE(duckdb_connect(database, &connection) == DuckDBSuccess);

    const auto consumer_table = table_name_for(0, "Consumer", "output");
    const auto aux_table = table_name_for(1, "Aux", "output");

    duckdb_result result;

    require_query(connection,
                  "SELECT timestamp_ns, simulation_time_s, model, storage, CPUtime, EventCounter, enabled, label FROM " +
                      quote_identifier(consumer_table) + " ORDER BY timestamp_ns;",
                  result);

    REQUIRE(duckdb_row_count(&result) == 1);
    REQUIRE(duckdb_value_int64(&result, 0, 0) == static_cast<std::int64_t>(timestamp));
    REQUIRE(duckdb_value_double(&result, 1, 0) == Catch::Approx(sim_time::ns_to_s(timestamp)));
    REQUIRE(std::string(duckdb_value_varchar_internal(&result, 2, 0)) == "Consumer");
    REQUIRE(std::string(duckdb_value_varchar_internal(&result, 3, 0)) == "output");
    REQUIRE(duckdb_value_double(&result, 4, 0) == Catch::Approx(cpu_time));
    REQUIRE(duckdb_value_int64(&result, 5, 0) == event_counter);
    REQUIRE(duckdb_value_boolean(&result, 6, 0));
    REQUIRE(std::string(duckdb_value_varchar_internal(&result, 7, 0)) == "hello");
    duckdb_destroy_result(&result);

    require_query(connection,
                  "SELECT timestamp_ns, simulation_time_s, model, storage, value FROM " +
                      quote_identifier(aux_table) + " ORDER BY timestamp_ns;",
                  result);

    REQUIRE(duckdb_row_count(&result) == 1);
    REQUIRE(duckdb_value_int64(&result, 0, 0) == static_cast<std::int64_t>(timestamp));
    REQUIRE(duckdb_value_double(&result, 1, 0) == Catch::Approx(sim_time::ns_to_s(timestamp)));
    REQUIRE(std::string(duckdb_value_varchar_internal(&result, 2, 0)) == "Aux");
    REQUIRE(std::string(duckdb_value_varchar_internal(&result, 3, 0)) == "output");
    REQUIRE(duckdb_value_double(&result, 4, 0) == Catch::Approx(aux_value));
    duckdb_destroy_result(&result);

    duckdb_disconnect(&connection);
    duckdb_close(&database);

    remove_if_exists(db_path);
}
