#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "signal/sinks/duckdb_recorder_sink.hpp"

#include "utils/time.hpp"

#include <duckdb.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>

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

    std::string table_name_from_metadata(duckdb_connection connection, const std::string &model, const std::string &storage_name)
    {
        duckdb_result result;
        require_query(connection,
                      "SELECT table_name FROM ssp4sim_metadata WHERE model = '" + model + "' AND storage_name = '" + storage_name + "';",
                      result);

        REQUIRE(duckdb_row_count(&result) == 1);
        const std::string table_name = duckdb_value_varchar_internal(&result, 0, 0);
        duckdb_destroy_result(&result);
        return table_name;
    }

    void record_single_consumer_value(const fs::path &db_path, double value, std::uint64_t timestamp)
    {
        DuckDbRecorderSink sink(db_path);

        SignalStorage storage(1, "Consumer.output");
        storage.add("Consumer.value", DataType::real, 1);
        storage.allocate();

        sink.on_storage_added(&storage);
        sink.init();
        sink.start();

        const std::size_t area = storage.push(timestamp);
        std::memcpy(storage.get_item(area, 0), &value, sizeof(double));

        NewDataEvent event;
        event.storage = &storage;
        event.area = area;
        event.timestamp = timestamp;
        event.buffer = storage.get_item(area, 0);
        event.recorder_storage_index = 0;

        REQUIRE_NOTHROW(sink.on_event(event));
        REQUIRE_NOTHROW(sink.stop());
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

    const auto consumer_table = table_name_from_metadata(connection, "Consumer", "output");
    const auto aux_table = table_name_from_metadata(connection, "Aux", "output");

    REQUIRE(consumer_table.rfind("Consumer_", 0) == 0);
    REQUIRE(aux_table.rfind("Aux_", 0) == 0);
    REQUIRE(consumer_table != aux_table);

    duckdb_result result;

    require_query(connection,
                  "SELECT table_name, model, storage_name, source_storage_name, created_at_s FROM ssp4sim_metadata ORDER BY model;",
                  result);

    REQUIRE(duckdb_row_count(&result) == 2);
    REQUIRE(std::string(duckdb_value_varchar_internal(&result, 1, 0)) == "Aux");
    REQUIRE(std::string(duckdb_value_varchar_internal(&result, 2, 0)) == "output");
    REQUIRE(std::string(duckdb_value_varchar_internal(&result, 3, 0)) == "Aux.output");
    REQUIRE(duckdb_value_int64(&result, 4, 0) > 0);
    REQUIRE(std::string(duckdb_value_varchar_internal(&result, 1, 1)) == "Consumer");
    REQUIRE(std::string(duckdb_value_varchar_internal(&result, 2, 1)) == "output");
    REQUIRE(std::string(duckdb_value_varchar_internal(&result, 3, 1)) == "Consumer.output");
    REQUIRE(duckdb_value_int64(&result, 4, 1) > 0);
    duckdb_destroy_result(&result);

    require_query(connection,
                  "SELECT timestamp_ns, simulation_time_s, CPUtime, EventCounter, enabled, label FROM " +
                      quote_identifier(consumer_table) + " ORDER BY timestamp_ns;",
                  result);

    REQUIRE(duckdb_row_count(&result) == 1);
    REQUIRE(duckdb_value_int64(&result, 0, 0) == static_cast<std::int64_t>(timestamp));
    REQUIRE(duckdb_value_double(&result, 1, 0) == Catch::Approx(sim_time::ns_to_s(timestamp)));
    REQUIRE(duckdb_value_double(&result, 2, 0) == Catch::Approx(cpu_time));
    REQUIRE(duckdb_value_int32(&result, 3, 0) == event_counter);
    REQUIRE(duckdb_value_boolean(&result, 4, 0));
    REQUIRE(std::string(duckdb_value_varchar_internal(&result, 5, 0)) == "hello");
    duckdb_destroy_result(&result);

    require_query(connection,
                  "SELECT timestamp_ns, simulation_time_s, value FROM " +
                      quote_identifier(aux_table) + " ORDER BY timestamp_ns;",
                  result);

    REQUIRE(duckdb_row_count(&result) == 1);
    REQUIRE(duckdb_value_int64(&result, 0, 0) == static_cast<std::int64_t>(timestamp));
    REQUIRE(duckdb_value_double(&result, 1, 0) == Catch::Approx(sim_time::ns_to_s(timestamp)));
    REQUIRE(duckdb_value_double(&result, 2, 0) == Catch::Approx(aux_value));
    duckdb_destroy_result(&result);

    duckdb_disconnect(&connection);
    duckdb_close(&database);

    remove_if_exists(db_path);
}

TEST_CASE("DuckDB recorder sink appends new runs to existing database", "[DataRecorder][DuckDB]")
{
    const auto db_path = test_path("test_duckdb_recorder_append.duckdb");
    remove_if_exists(db_path);

    record_single_consumer_value(db_path, 1.25, 1ULL * sim_time::nanoseconds_per_second);

    duckdb_database database = nullptr;
    duckdb_connection connection = nullptr;
    REQUIRE(duckdb_open(db_path.string().c_str(), &database) == DuckDBSuccess);
    REQUIRE(duckdb_connect(database, &connection) == DuckDBSuccess);

    duckdb_result result;
    require_query(connection,
                  "SELECT table_name FROM ssp4sim_metadata WHERE model = 'Consumer' AND storage_name = 'output';",
                  result);
    REQUIRE(duckdb_row_count(&result) == 1);
    const std::string first_table = duckdb_value_varchar_internal(&result, 0, 0);
    duckdb_destroy_result(&result);

    duckdb_disconnect(&connection);
    duckdb_close(&database);

    record_single_consumer_value(db_path, 2.5, 2ULL * sim_time::nanoseconds_per_second);

    database = nullptr;
    connection = nullptr;
    REQUIRE(duckdb_open(db_path.string().c_str(), &database) == DuckDBSuccess);
    REQUIRE(duckdb_connect(database, &connection) == DuckDBSuccess);

    require_query(connection,
                  "SELECT table_name FROM ssp4sim_metadata WHERE model = 'Consumer' AND storage_name = 'output';",
                  result);
    REQUIRE(duckdb_row_count(&result) == 2);
    const std::string first_row_table = duckdb_value_varchar_internal(&result, 0, 0);
    const std::string second_row_table = duckdb_value_varchar_internal(&result, 0, 1);
    const std::string second_table = first_row_table == first_table ? second_row_table : first_row_table;
    REQUIRE(first_table != second_table);
    duckdb_destroy_result(&result);

    require_query(connection,
                  "SELECT value FROM " + quote_identifier(first_table) + ";",
                  result);
    REQUIRE(duckdb_row_count(&result) == 1);
    REQUIRE(duckdb_value_double(&result, 0, 0) == Catch::Approx(1.25));
    duckdb_destroy_result(&result);

    require_query(connection,
                  "SELECT value FROM " + quote_identifier(second_table) + ";",
                  result);
    REQUIRE(duckdb_row_count(&result) == 1);
    REQUIRE(duckdb_value_double(&result, 0, 0) == Catch::Approx(2.5));
    duckdb_destroy_result(&result);

    duckdb_disconnect(&connection);
    duckdb_close(&database);

    remove_if_exists(db_path);
}
