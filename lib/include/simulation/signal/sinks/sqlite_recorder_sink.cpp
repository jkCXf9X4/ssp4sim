#include "signal/sinks/sqlite_recorder_sink.hpp"
#include "signal/sinks/sqlite_recorder_utils.hpp"

#include "utils/io.hpp"
#include "utils/time.hpp"

#include <sqlite3.h>

#include <chrono>
#include <cstdint>
#include <format>
#include <stdexcept>
#include <string>

namespace ssp4sim::signal
{
    namespace
    {
        std::string sqlite_exec_error(sqlite3 *db, char *error_message)
        {
            if (error_message != nullptr)
            {
                std::string message(error_message);
                sqlite3_free(error_message);
                return message;
            }

            return db == nullptr ? "unknown SQLite error" : sqlite3_errmsg(db);
        }

        // TODO: Move to some util...
        std::int64_t current_epoch_seconds()
        {
            using namespace std::chrono;
            const auto now = system_clock::now();
            return duration_cast<seconds>(now.time_since_epoch()).count();
        }
    }

    SqliteWALRecorderSink::SqliteWALRecorderSink(std::filesystem::path working_dir, std::string session_uuid, std::optional<std::filesystem::path> file_override)
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.signal.SqliteWALRecorderSink")),
          working_dir(std::move(working_dir)),
          session_uuid(std::move(session_uuid)),
          file_override(std::move(file_override))
    {
        LOG_DEBUG(log, "[{func}] Working dir {dir}", __func__, this->working_dir.string());
    }

    SqliteWALRecorderSink::~SqliteWALRecorderSink()
    {
        stop();
    }

    void SqliteWALRecorderSink::open_database()
    {
        if (file_override.has_value())
        {
            filename = file_override.value();
        }
        else
        {
            const auto epoch = current_epoch_seconds();
            filename = working_dir / std::format("{}_{}.sqlite", epoch, session_uuid);
        }

        utils::io::create_parent_folder(filename.string());

        auto rc = sqlite3_open_v2(
            filename.string().c_str(),
            &db,
            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX,
            nullptr);
        if (rc != SQLITE_OK)
        {
            const std::string msg = "Failed to open SQLite database: " + std::string(sqlite3_errmsg(db));
            if (db != nullptr)
            {
                sqlite3_close(db);
                db = nullptr;
            }
            throw std::runtime_error(msg);
        }

        // Enable WAL mode for concurrent readers. NORMAL synchronous avoids the
        // high per-commit fsync cost of FULL while keeping WAL transactions atomic.
        // Checkpointing is deferred until stop so it does not interrupt the hot path.
        execute_pragma("PRAGMA journal_mode=WAL;", "Failed to enable WAL mode");
        execute_pragma("PRAGMA synchronous=NORMAL;", "Failed to configure SQLite synchronous mode");
        execute_pragma("PRAGMA wal_autocheckpoint=0;", "Failed to disable SQLite automatic WAL checkpointing");
        execute_pragma("PRAGMA temp_store=MEMORY;", "Failed to configure SQLite temporary storage");
        execute_pragma("PRAGMA cache_size=-65536;", "Failed to configure SQLite cache size");

        // Create run_counter table if it does not exist, then atomically increment
        {
            const std::string sql = "CREATE TABLE IF NOT EXISTS ssp4sim_run_counter (run_id INTEGER PRIMARY KEY);";
            char *error_message = nullptr;
            rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &error_message);
            if (rc != SQLITE_OK)
            {
                const std::string msg = "Failed to create run_counter table: " + std::string(error_message);
                sqlite3_free(error_message);
                sqlite3_close(db);
                db = nullptr;
                throw std::runtime_error(msg);
            }
        }

        run_id = sqlite_recorder::run_counter(db);
        LOG_DEBUG(log, "[{func}] Run ID {run_id}", __func__, run_id);
    }

    void SqliteWALRecorderSink::execute_pragma(const char *sql, const char *context)
    {
        char *error_message = nullptr;
        const auto rc = sqlite3_exec(db, sql, nullptr, nullptr, &error_message);
        if (rc != SQLITE_OK)
        {
            const std::string msg = std::string(context) + ": " + sqlite_exec_error(db, error_message);
            sqlite3_close(db);
            db = nullptr;
            throw std::runtime_error(msg);
        }
    }

    void SqliteWALRecorderSink::begin_transaction()
    {
        char *error_message = nullptr;
        const auto rc = sqlite3_exec(db, "BEGIN IMMEDIATE;", nullptr, nullptr, &error_message);
        if (rc != SQLITE_OK)
        {
            const std::string msg = "Failed to begin transaction: " + sqlite_exec_error(db, error_message);
            disable_sink(msg);
        }
    }

    void SqliteWALRecorderSink::commit_transaction(const char *context)
    {
        char *error_message = nullptr;
        const auto rc = sqlite3_exec(db, "COMMIT;", nullptr, nullptr, &error_message);
        if (rc != SQLITE_OK)
        {
            const std::string msg = std::string(context) + ": " + sqlite_exec_error(db, error_message);
            disable_sink(msg);
            return;
        }

        insert_count = 0;
    }

    std::string SqliteWALRecorderSink::local_variable_name(const std::string &storage_model, const std::string &name)
    {
        const auto prefix = storage_model.empty() ? std::string{} : storage_model + '.';
        if (!prefix.empty() && name.rfind(prefix, 0) == 0 && name.size() > prefix.size())
        {
            return name.substr(prefix.size());
        }

        return name;
    }

    void SqliteWALRecorderSink::on_storage_added(const SignalStorage *storage)
    {
        if (storage == nullptr || storage->mem_size == 0)
        {
            return;
        }

        auto [model, storage_name] = sqlite_recorder::split_storage_name(storage->name);

        SqliteStorageLayout layout;
        layout.storage = storage;
        layout.index = layouts.size();
        layout.model = std::move(model);
        layout.storage_name = std::move(storage_name);
        layout.variables.reserve(storage->variables.size());

        for (std::size_t i = 0; i < storage->variables.size(); ++i)
        {
            const auto &variable = storage->variables[i];
            SqliteVariableLayout variable_layout;
            variable_layout.name = local_variable_name(layout.model, variable.name);
            variable_layout.type = variable.type;
            variable_layout.position = variable.position;
            variable_layout.bind_index = static_cast<int>(i) + 3;
            layout.variables.emplace_back(std::move(variable_layout));
        }

        layout_lookup[storage] = layout.index;
        layouts.emplace_back(std::move(layout));

        if (initialized && !disabled)
        {
            try
            {
                open_layout(layouts.back());
            }
            catch (const std::exception &e)
            {
                disable_sink(e.what());
            }
        }
    }

    void SqliteWALRecorderSink::open_layout(SqliteStorageLayout &layout)
    {
        // Compute table name using run_id (now available after open_database)
        layout.table_name = sqlite_recorder::table_name_for(run_id, layout.model, layout.storage_name);

        // CREATE TABLE IF NOT EXISTS
        std::string create_sql = "CREATE TABLE IF NOT EXISTS ";
        create_sql += sqlite_recorder::quote_identifier(layout.table_name);
        create_sql += " (";
        create_sql += sqlite_recorder::quote_identifier("timestamp_ns");
        create_sql += " INTEGER NOT NULL, ";
        create_sql += sqlite_recorder::quote_identifier("simulation_time_s");
        create_sql += " REAL NOT NULL";

        for (const auto &variable : layout.variables)
        {
            create_sql += ", ";
            create_sql += sqlite_recorder::quote_identifier(variable.name);
            create_sql += ' ';
            create_sql += sqlite_recorder::sql_type_for(variable.type);
            create_sql += " NOT NULL";
        }

        create_sql += ");";

        char *error_message = nullptr;
        auto rc = sqlite3_exec(db, create_sql.c_str(), nullptr, nullptr, &error_message);
        if (rc != SQLITE_OK)
        {
            const std::string msg = "Failed to create SQLite table: " + std::string(error_message);
            sqlite3_free(error_message);
            throw std::runtime_error(msg);
        }

        // Prepare INSERT INTO statement
        std::string insert_sql = "INSERT INTO ";
        insert_sql += sqlite_recorder::quote_identifier(layout.table_name);
        insert_sql += " (";
        insert_sql += sqlite_recorder::quote_identifier("timestamp_ns");
        insert_sql += ", ";
        insert_sql += sqlite_recorder::quote_identifier("simulation_time_s");

        for (const auto &variable : layout.variables)
        {
            insert_sql += ", ";
            insert_sql += sqlite_recorder::quote_identifier(variable.name);
        }

        insert_sql += ") VALUES (?1, ?2";

        for (std::size_t i = 0; i < layout.variables.size(); ++i)
        {
            insert_sql += ", ?" + std::to_string(static_cast<int>(i) + 3);
        }

        insert_sql += ");";

        rc = sqlite3_prepare_v2(db, insert_sql.c_str(), static_cast<int>(insert_sql.size()), &layout.insert_stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            throw std::runtime_error("Failed to prepare SQLite insert statement: " + std::string(sqlite3_errmsg(db)));
        }
    }

    void SqliteWALRecorderSink::init()
    {
        LOG_TRACE_L1(log, "[{func}] Init", __func__);
        if (disabled || stopped)
        {
            return;
        }

        try
        {
            open_database();
            initialized = true;
            for (auto &layout : layouts)
            {
                open_layout(layout);
            }
        }
        catch (const std::exception &e)
        {
            disable_sink(e.what());
        }
    }

    void SqliteWALRecorderSink::on_event(const NewDataEvent &event)
    {
        if (disabled || stopped || event.storage == nullptr || event.buffer == nullptr)
        {
            return;
        }

        auto layout_it = layout_lookup.find(event.storage);
        if (layout_it == layout_lookup.end())
        {
            LOG_WARNING(log, "[{func}] Ignoring event for unknown storage {}", __func__, event.storage->name);
            return;
        }

        auto &layout = layouts[layout_it->second];
        if (layout.insert_stmt == nullptr)
        {
            LOG_WARNING(log, "[{func}] Ignoring event for unopened storage {}", __func__, event.storage->name);
            return;
        }

        if (insert_count == 0)
        {
            begin_transaction();
            if (disabled)
            {
                return;
            }
        }

        const auto simulation_time_s = utils::time::ns_to_s(event.timestamp);
        const auto timestamp_ns = static_cast<std::int64_t>(event.timestamp);

        auto *stmt = layout.insert_stmt;

        try
        {
            sqlite_recorder::check_sqlite_error(
                sqlite3_bind_int64(stmt, 1, timestamp_ns),
                "Failed to bind timestamp");

            sqlite_recorder::check_sqlite_error(
                sqlite3_bind_double(stmt, 2, simulation_time_s),
                "Failed to bind simulation time");

            for (const auto &variable : layout.variables)
            {
                const auto *data = event.buffer + variable.position;

                switch (static_cast<types::DataType::Value>(variable.type))
                {
                case types::DataType::Value::real:
                    sqlite_recorder::check_sqlite_error(
                        sqlite3_bind_double(stmt, variable.bind_index, *reinterpret_cast<const double *>(data)),
                        "Failed to bind real value");
                    break;
                case types::DataType::Value::integer:
                case types::DataType::Value::enumeration:
                    sqlite_recorder::check_sqlite_error(
                        sqlite3_bind_int(stmt, variable.bind_index, *reinterpret_cast<const int *>(data)),
                        "Failed to bind integer value");
                    break;
                case types::DataType::Value::boolean:
                    sqlite_recorder::check_sqlite_error(
                        sqlite3_bind_int(stmt, variable.bind_index, (*reinterpret_cast<const int *>(data) != 0) ? 1 : 0),
                        "Failed to bind boolean value");
                    break;
                case types::DataType::Value::string:
                    sqlite_recorder::check_sqlite_error(
                        sqlite3_bind_text(stmt, variable.bind_index, reinterpret_cast<const std::string *>(data)->c_str(),
                                          static_cast<int>(reinterpret_cast<const std::string *>(data)->size()),
                                          SQLITE_TRANSIENT),
                        "Failed to bind string value");
                    break;
                default:
                    throw std::runtime_error("Unsupported data type for SQLite recording");
                }
            }

            auto rc = sqlite3_step(stmt);
            if (rc != SQLITE_DONE)
            {
                throw std::runtime_error("Failed to step SQLite insert: " + std::string(sqlite3_errmsg(db)));
            }

            sqlite_recorder::check_sqlite_error(
                sqlite3_reset(stmt),
                "Failed to reset SQLite statement");

            ++insert_count;

            if (insert_count >= commit_interval)
            {
                commit_transaction("Failed to commit transaction");
            }
        }
        catch (const std::exception &e)
        {
            disable_sink(e.what());
        }
    }

    void SqliteWALRecorderSink::disable_sink(const std::string &reason)
    {
        if (disabled)
        {
            return;
        }

        disabled = true;
        stopped = true;
        LOG_WARNING(log, "[{func}] SQLite sink disabled: {}", __func__, reason);

        for (auto &layout : layouts)
        {
            if (layout.insert_stmt != nullptr)
            {
                sqlite3_finalize(layout.insert_stmt);
                layout.insert_stmt = nullptr;
            }
        }

        if (db != nullptr)
        {
            sqlite3_close(db);
            db = nullptr;
        }
    }

    void SqliteWALRecorderSink::stop()
    {
        if (disabled || stopped)
        {
            return;
        }

        // Commit any remaining rows
        if (insert_count > 0)
        {
            commit_transaction("Failed to commit final transaction");
            if (disabled)
            {
                return;
            }
        }

        try
        {
            for (auto &layout : layouts)
            {
                if (layout.insert_stmt != nullptr)
                {
                    sqlite_recorder::check_sqlite_error(
                        sqlite3_finalize(layout.insert_stmt),
                        "Failed to finalize SQLite statement");
                    layout.insert_stmt = nullptr;
                }
            }
        }
        catch (const std::exception &e)
        {
            disable_sink(e.what());
            return;
        }

        if (db != nullptr)
        {
            sqlite3_exec(db, "PRAGMA wal_checkpoint(TRUNCATE);", nullptr, nullptr, nullptr);
            sqlite3_close(db);
            db = nullptr;
        }

        stopped = true;
    }
}
