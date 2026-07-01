#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include "simulation/signal/sinks/sqlite_recorder_sink.hpp"

#include "utils/time/time.hpp"

#include <sqlite3.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <string>
#include <thread>

namespace fs = std::filesystem;
namespace sim_time = ssp4sim::utils::time;

using ssp4sim::signal::SqliteWALRecorderSink;
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

    struct SqliteHelper
    {
        sqlite3 *db = nullptr;

        void open(const fs::path &path)
        {
            REQUIRE(sqlite3_open_v2(path.string().c_str(), &db, SQLITE_OPEN_READWRITE, nullptr) == SQLITE_OK);
        }

        void close()
        {
            if (db != nullptr)
            {
                sqlite3_close(db);
                db = nullptr;
            }
        }

        ~SqliteHelper()
        {
            close();
        }

        int exec(const std::string &sql)
        {
            char *error_message = nullptr;
            const auto rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &error_message);
            if (rc != SQLITE_OK)
            {
                FAIL("SQLite exec failed: " << sql << " : " << error_message);
                sqlite3_free(error_message);
            }
            return rc;
        }

        std::string exec_and_return_string(const std::string &sql, int col)
        {
            sqlite3_stmt *stmt = nullptr;
            REQUIRE(sqlite3_prepare_v2(db, sql.c_str(), static_cast<int>(sql.size()), &stmt, nullptr) == SQLITE_OK);
            REQUIRE(sqlite3_step(stmt) == SQLITE_ROW);
            const std::string result = reinterpret_cast<const char *>(sqlite3_column_text(stmt, col));
            sqlite3_finalize(stmt);
            return result;
        }

        std::int64_t exec_and_return_int64(const std::string &sql, int col)
        {
            sqlite3_stmt *stmt = nullptr;
            REQUIRE(sqlite3_prepare_v2(db, sql.c_str(), static_cast<int>(sql.size()), &stmt, nullptr) == SQLITE_OK);
            REQUIRE(sqlite3_step(stmt) == SQLITE_ROW);
            const auto result = sqlite3_column_int64(stmt, col);
            sqlite3_finalize(stmt);
            return result;
        }

        int exec_and_return_int(const std::string &sql, int col)
        {
            sqlite3_stmt *stmt = nullptr;
            REQUIRE(sqlite3_prepare_v2(db, sql.c_str(), static_cast<int>(sql.size()), &stmt, nullptr) == SQLITE_OK);
            REQUIRE(sqlite3_step(stmt) == SQLITE_ROW);
            const auto result = sqlite3_column_int(stmt, col);
            sqlite3_finalize(stmt);
            return result;
        }

        double exec_and_return_double(const std::string &sql, int col)
        {
            sqlite3_stmt *stmt = nullptr;
            REQUIRE(sqlite3_prepare_v2(db, sql.c_str(), static_cast<int>(sql.size()), &stmt, nullptr) == SQLITE_OK);
            REQUIRE(sqlite3_step(stmt) == SQLITE_ROW);
            const auto result = sqlite3_column_double(stmt, col);
            sqlite3_finalize(stmt);
            return result;
        }

        int row_count(const std::string &sql)
        {
            sqlite3_stmt *stmt = nullptr;
            REQUIRE(sqlite3_prepare_v2(db, sql.c_str(), static_cast<int>(sql.size()), &stmt, nullptr) == SQLITE_OK);
            int count = 0;
            while (sqlite3_step(stmt) == SQLITE_ROW)
            {
                ++count;
            }
            sqlite3_finalize(stmt);
            return count;
        }

        std::int64_t run_id()
        {
            return exec_and_return_int64("SELECT MAX(run_id) FROM ssp4sim_run_counter;", 0);
        }

        std::string table_name_from_master(std::int64_t run_id, const std::string &model, const std::string &storage_name)
        {
            const std::string expected = "I" + std::to_string(run_id) + "_" + model + "_" + storage_name;
            return exec_and_return_string(
                "SELECT name FROM sqlite_master WHERE type='table' AND name = '" + expected + "';",
                0);
        }
    };

    void record_single_consumer_value(const fs::path &db_path, double value, std::uint64_t timestamp)
    {
        SqliteWALRecorderSink sink(fs::temp_directory_path(), "test-uuid", db_path);

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

TEST_CASE("T-001: SQLite sink writes events with mixed types and verifies via SELECT", "[DataRecorder][SQLite]")
{
    const auto db_path = test_path("test_sqlite_recorder.sqlite");
    remove_if_exists(db_path);

    SqliteWALRecorderSink sink(fs::temp_directory_path(), "test-uuid", db_path);

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

    SqliteHelper db;
    db.open(db_path);

    const auto consumer_table = db.table_name_from_master(1, "Consumer", "output");
    const auto aux_table = db.table_name_from_master(1, "Aux", "output");

    REQUIRE(consumer_table == "I1_Consumer_output");
    REQUIRE(aux_table == "I1_Aux_output");
    REQUIRE(consumer_table != aux_table);

    // Verify NO ssp4sim_metadata table exists
    REQUIRE(db.row_count("SELECT name FROM sqlite_master WHERE type='table' AND name='ssp4sim_metadata';") == 0);

    // Check consumer table data
    REQUIRE(db.row_count("SELECT * FROM " + quote_identifier(consumer_table) + " ORDER BY timestamp_ns;") == 1);
    REQUIRE(db.exec_and_return_int64("SELECT timestamp_ns FROM " + quote_identifier(consumer_table) + " ORDER BY timestamp_ns;", 0) == static_cast<std::int64_t>(timestamp));
    REQUIRE(db.exec_and_return_double("SELECT simulation_time_s FROM " + quote_identifier(consumer_table) + " ORDER BY timestamp_ns;", 0) == Catch::Approx(sim_time::ns_to_s(timestamp)));
    REQUIRE(db.exec_and_return_double("SELECT CPUtime FROM " + quote_identifier(consumer_table) + " ORDER BY timestamp_ns;", 0) == Catch::Approx(cpu_time));
    REQUIRE(db.exec_and_return_int("SELECT EventCounter FROM " + quote_identifier(consumer_table) + " ORDER BY timestamp_ns;", 0) == event_counter);
    REQUIRE(db.exec_and_return_int("SELECT enabled FROM " + quote_identifier(consumer_table) + " ORDER BY timestamp_ns;", 0) == 1);
    REQUIRE(db.exec_and_return_string("SELECT label FROM " + quote_identifier(consumer_table) + " ORDER BY timestamp_ns;", 0) == "hello");

    // Check aux table data
    REQUIRE(db.row_count("SELECT * FROM " + quote_identifier(aux_table) + " ORDER BY timestamp_ns;") == 1);
    REQUIRE(db.exec_and_return_double("SELECT value FROM " + quote_identifier(aux_table) + " ORDER BY timestamp_ns;", 0) == Catch::Approx(aux_value));

    db.close();
    remove_if_exists(db_path);
}

TEST_CASE("T-002: SQLite sink verifies PRAGMA journal_mode=wal", "[DataRecorder][SQLite]")
{
    const auto db_path = test_path("test_sqlite_recorder_wal.sqlite");
    remove_if_exists(db_path);

    {
        SqliteWALRecorderSink sink(fs::temp_directory_path(), "test-uuid", db_path);

        SignalStorage storage(1, "Test.model");
        storage.add("Test.value", DataType::real, 1);
        storage.allocate();

        sink.on_storage_added(&storage);
        sink.init();
        sink.start();

        const auto timestamp = 1ULL * sim_time::nanoseconds_per_second;
        const std::size_t area = storage.push(timestamp);
        const double val = 42.0;
        std::memcpy(storage.get_item(area, 0), &val, sizeof(double));

        NewDataEvent event;
        event.storage = &storage;
        event.area = area;
        event.timestamp = timestamp;
        event.buffer = storage.get_item(area, 0);
        event.recorder_storage_index = 0;

        REQUIRE_NOTHROW(sink.on_event(event));
        REQUIRE_NOTHROW(sink.stop());
    }

    REQUIRE(fs::exists(db_path));

    // Verify journal mode via PRAGMA
    sqlite3 *verify_db = nullptr;
    REQUIRE(sqlite3_open_v2(db_path.string().c_str(), &verify_db, SQLITE_OPEN_READWRITE, nullptr) == SQLITE_OK);

    sqlite3_stmt *stmt = nullptr;
    REQUIRE(sqlite3_prepare_v2(verify_db, "PRAGMA journal_mode;", -1, &stmt, nullptr) == SQLITE_OK);
    REQUIRE(sqlite3_step(stmt) == SQLITE_ROW);
    const std::string journal_mode = reinterpret_cast<const char *>(sqlite3_column_text(stmt, 0));
    sqlite3_finalize(stmt);
    sqlite3_close(verify_db);

    REQUIRE(journal_mode == "wal");

    remove_if_exists(db_path);
}

TEST_CASE("T-003: SQLite run counter and no metadata table", "[DataRecorder][SQLite]")
{
    const auto db_path = test_path("test_sqlite_recorder_run_counter.sqlite");
    remove_if_exists(db_path);

    {
        SqliteWALRecorderSink sink(fs::temp_directory_path(), "test-uuid", db_path);

        SignalStorage storage(1, "Consumer.output");
        storage.add("Consumer.value", DataType::real, 1);
        storage.allocate();

        sink.on_storage_added(&storage);
        sink.init();
        sink.start();

        const auto timestamp = 1ULL * sim_time::nanoseconds_per_second;
        const std::size_t area = storage.push(timestamp);
        const double val = 3.14;
        std::memcpy(storage.get_item(area, 0), &val, sizeof(double));

        NewDataEvent event;
        event.storage = &storage;
        event.area = area;
        event.timestamp = timestamp;
        event.buffer = storage.get_item(area, 0);
        event.recorder_storage_index = 0;

        REQUIRE_NOTHROW(sink.on_event(event));
        REQUIRE_NOTHROW(sink.stop());
    }

    SqliteHelper db;
    db.open(db_path);

    // Verify ssp4sim_run_counter exists with run_id=1
    REQUIRE(db.row_count("SELECT name FROM sqlite_master WHERE type='table' AND name='ssp4sim_run_counter';") == 1);
    REQUIRE(db.run_id() == 1);

    // Verify NO ssp4sim_metadata exists
    REQUIRE(db.row_count("SELECT name FROM sqlite_master WHERE type='table' AND name='ssp4sim_metadata';") == 0);

    // Verify consumer table via sqlite_master
    const std::string table_name = db.table_name_from_master(1, "Consumer", "output");
    REQUIRE(table_name == "I1_Consumer_output");

    // Verify data values
    REQUIRE(db.row_count("SELECT * FROM " + quote_identifier(table_name) + ";") == 1);
    REQUIRE(db.exec_and_return_double("SELECT value FROM " + quote_identifier(table_name) + ";", 0) == Catch::Approx(3.14));

    db.close();
    remove_if_exists(db_path);
}

TEST_CASE("T-004: SQLite sink appends runs to existing database (shared-file mode)", "[DataRecorder][SQLite]")
{
    const auto db_path = test_path("test_sqlite_recorder_append.sqlite");
    remove_if_exists(db_path);

    record_single_consumer_value(db_path, 1.25, 1ULL * sim_time::nanoseconds_per_second);

    // Check first run
    SqliteHelper db;
    db.open(db_path);
    const std::string first_table = db.table_name_from_master(1, "Consumer", "output");
    REQUIRE(first_table == "I1_Consumer_output");
    db.close();

    // Second run - appends to same shared file
    record_single_consumer_value(db_path, 2.5, 2ULL * sim_time::nanoseconds_per_second);

    db.open(db_path);
    const std::string second_table = db.table_name_from_master(2, "Consumer", "output");
    REQUIRE(second_table == "I2_Consumer_output");

    // Both tables should exist
    REQUIRE(db.row_count("SELECT name FROM sqlite_master WHERE type='table' AND name = 'I1_Consumer_output';") == 1);
    REQUIRE(db.row_count("SELECT name FROM sqlite_master WHERE type='table' AND name = 'I2_Consumer_output';") == 1);

    // Verify each table has its own data
    REQUIRE(db.row_count("SELECT value FROM " + quote_identifier(first_table) + ";") == 1);
    REQUIRE(db.exec_and_return_double("SELECT value FROM " + quote_identifier(first_table) + ";", 0) == Catch::Approx(1.25));

    REQUIRE(db.row_count("SELECT value FROM " + quote_identifier(second_table) + ";") == 1);
    REQUIRE(db.exec_and_return_double("SELECT value FROM " + quote_identifier(second_table) + ";", 0) == Catch::Approx(2.5));

    // Run counter should be 2
    REQUIRE(db.run_id() == 2);

    db.close();
    remove_if_exists(db_path);
}

TEST_CASE("T-005: Unknown storage event does not crash SQLite sink", "[DataRecorder][SQLite]")
{
    const auto db_path = test_path("test_sqlite_recorder_unknown.sqlite");
    remove_if_exists(db_path);

    SqliteWALRecorderSink sink(fs::temp_directory_path(), "test-uuid", db_path);

    SignalStorage storage(1, "Consumer.output");
    storage.add("Consumer.value", DataType::real, 1);
    storage.allocate();

    sink.on_storage_added(&storage);
    sink.init();
    sink.start();

    // Create an event referencing a storage that was never registered
    SignalStorage unknown_storage(1, "Unknown.output");
    unknown_storage.add("Unknown.value", DataType::real, 1);
    unknown_storage.allocate();

    const auto timestamp = 1ULL * sim_time::nanoseconds_per_second;
    const std::size_t area = unknown_storage.push(timestamp);
    const double val = 99.9;
    std::memcpy(unknown_storage.get_item(area, 0), &val, sizeof(double));

    NewDataEvent event;
    event.storage = &unknown_storage;
    event.area = area;
    event.timestamp = timestamp;
    event.buffer = unknown_storage.get_item(area, 0);
    event.recorder_storage_index = 0;

    // Must not crash or throw
    REQUIRE_NOTHROW(sink.on_event(event));

    // Known storage should still work
    const std::size_t known_area = storage.push(timestamp);
    std::memcpy(storage.get_item(known_area, 0), &val, sizeof(double));

    NewDataEvent known_event;
    known_event.storage = &storage;
    known_event.area = known_area;
    known_event.timestamp = timestamp;
    known_event.buffer = storage.get_item(known_area, 0);
    known_event.recorder_storage_index = 0;

    REQUIRE_NOTHROW(sink.on_event(known_event));

    REQUIRE_NOTHROW(sink.stop());

    // Verify only the known storage data was recorded
    SqliteHelper db;
    db.open(db_path);
    const auto consumer_table = db.table_name_from_master(1, "Consumer", "output");

    REQUIRE(db.row_count("SELECT * FROM " + quote_identifier(consumer_table) + ";") == 1);
    REQUIRE(db.exec_and_return_double("SELECT value FROM " + quote_identifier(consumer_table) + ";", 0) == Catch::Approx(99.9));

    db.close();
    remove_if_exists(db_path);
}

TEST_CASE("T-006: Concurrent read while SQLite sink writes", "[DataRecorder][SQLite]")
{
    const auto db_path = test_path("test_sqlite_recorder_concurrent.sqlite");
    remove_if_exists(db_path);

    sqlite3 *reader_db = nullptr;
    REQUIRE(sqlite3_open_v2(db_path.string().c_str(), &reader_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, nullptr) == SQLITE_OK);
    sqlite3_close(reader_db);
    reader_db = nullptr;

    SqliteWALRecorderSink sink(fs::temp_directory_path(), "test-uuid", db_path);

    SignalStorage storage(1, "Consumer.output");
    storage.add("Consumer.value", DataType::real, 1);
    storage.allocate();

    sink.on_storage_added(&storage);
    sink.init();
    sink.start();

    // Write enough events to ensure at least one commit boundary is crossed.
    for (int i = 0; i < 10050; ++i)
    {
        const auto timestamp = static_cast<std::uint64_t>(i + 1) * sim_time::nanoseconds_per_second;
        const std::size_t area = storage.push(timestamp);
        const double val = static_cast<double>(i);
        std::memcpy(storage.get_item(area, 0), &val, sizeof(double));

        NewDataEvent event;
        event.storage = &storage;
        event.area = area;
        event.timestamp = timestamp;
        event.buffer = storage.get_item(area, 0);
        event.recorder_storage_index = 0;

        REQUIRE_NOTHROW(sink.on_event(event));
    }

    // Open a second connection for concurrent reading while writer is still open
    REQUIRE(sqlite3_open_v2(db_path.string().c_str(), &reader_db, SQLITE_OPEN_READWRITE, nullptr) == SQLITE_OK);

    // Look up consumer table name from sqlite_master
    std::string consumer_table;
    {
        sqlite3_stmt *meta_stmt = nullptr;
        REQUIRE(sqlite3_prepare_v2(reader_db,
            "SELECT name FROM sqlite_master WHERE type='table' AND name LIKE 'I1_Consumer_output';",
            -1, &meta_stmt, nullptr) == SQLITE_OK);
        REQUIRE(sqlite3_step(meta_stmt) == SQLITE_ROW);
        consumer_table = reinterpret_cast<const char *>(sqlite3_column_text(meta_stmt, 0));
        sqlite3_finalize(meta_stmt);
    }

    // Should be able to read at least some committed rows
    {
        sqlite3_stmt *count_stmt = nullptr;
        const std::string count_sql = "SELECT COUNT(*) FROM " + quote_identifier(consumer_table) + ";";
        REQUIRE(sqlite3_prepare_v2(reader_db, count_sql.c_str(), static_cast<int>(count_sql.size()), &count_stmt, nullptr) == SQLITE_OK);
        REQUIRE(sqlite3_step(count_stmt) == SQLITE_ROW);
        const int committed_count = sqlite3_column_int(count_stmt, 0);
        sqlite3_finalize(count_stmt);

        // At least the first commit batch should be visible.
        REQUIRE(committed_count >= 10000);
    }

    sqlite3_close(reader_db);

    REQUIRE_NOTHROW(sink.stop());

    remove_if_exists(db_path);
}

TEST_CASE("T-007: Row-count match for SQLite events", "[DataRecorder][SQLite]")
{
    const auto sqlite_path = test_path("test_row_count_match.sqlite");
    remove_if_exists(sqlite_path);

    // Write to SQLite
    {
        SqliteWALRecorderSink sink(fs::temp_directory_path(), "test-uuid", sqlite_path);

        SignalStorage storage(1, "Consumer.output");
        storage.add("Consumer.CPUtime", DataType::real, 1);
        storage.add("Consumer.EventCounter", DataType::integer, 1);
        storage.add("Consumer.enabled", DataType::boolean, 1);
        storage.add("Consumer.label", DataType::string, 1);
        storage.allocate();

        sink.on_storage_added(&storage);
        sink.init();
        sink.start();

        for (int i = 0; i < 10; ++i)
        {
            const auto timestamp = static_cast<std::uint64_t>(i + 1) * sim_time::nanoseconds_per_second;
            const std::size_t area = storage.push(timestamp);
            const double cpu_time = static_cast<double>(i) * 0.1;
            const int event_counter = i;
            const int enabled = (i % 2);
            const std::string label = "event_" + std::to_string(i);

            std::memcpy(storage.get_item(area, 0), &cpu_time, sizeof(double));
            std::memcpy(storage.get_item(area, 1), &event_counter, sizeof(int));
            std::memcpy(storage.get_item(area, 2), &enabled, sizeof(int));
            auto *label_ptr = reinterpret_cast<std::string *>(storage.get_item(area, 3));
            *label_ptr = label;

            NewDataEvent event;
            event.storage = &storage;
            event.area = area;
            event.timestamp = timestamp;
            event.buffer = storage.get_item(area, 0);
            event.recorder_storage_index = 0;

            REQUIRE_NOTHROW(sink.on_event(event));
        }

        REQUIRE_NOTHROW(sink.stop());
    }

    // Verify SQLite row count
    SqliteHelper db;
    db.open(sqlite_path);

    const auto consumer_table = db.table_name_from_master(1, "Consumer", "output");
    REQUIRE(db.row_count("SELECT * FROM " + quote_identifier(consumer_table) + ";") == 10);

    db.close();
    remove_if_exists(sqlite_path);
}
