#include "signal/sinks/duckdb_recorder_sink.hpp"
#include "signal/sinks/duckdb_recorder_utils.hpp"

#include "utils/io.hpp"
#include "utils/time.hpp"

#include <duckdb.h>

#include <cstdint>
#include <stdexcept>

namespace ssp4sim::signal
{
    DuckDbRecorderSink::DuckDbRecorderSink(const std::filesystem::path &filename)
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.signal.DuckDbRecorderSink")),
          filename(filename)
    {
        LOG_DEBUG(log, "[{func}] File {file}", __func__, filename.string());
    }

    DuckDbRecorderSink::~DuckDbRecorderSink()
    {
        stop();
    }

    void DuckDbRecorderSink::open_database()
    {
        utils::io::create_parent_folder(filename.string());
        duckdb_recorder::check_duckdb_state(duckdb_open(filename.string().c_str(), &database), "Failed to open DuckDB database");
        duckdb_recorder::check_duckdb_state(duckdb_connect(database, &connection), "Failed to connect to DuckDB database");
        duckdb_recorder::create_metadata_table(connection);
    }

    void DuckDbRecorderSink::on_storage_added(const SignalStorage *storage)
    {
        if (storage == nullptr || storage->mem_size == 0)
        {
            return;
        }

        auto [model, storage_name] = duckdb_recorder::split_storage_name(storage->name);

        DuckDbStorageLayout layout;
        layout.storage = storage;
        layout.index = layouts.size();
        layout.model = std::move(model);
        layout.storage_name = std::move(storage_name);
        layout.table_name = duckdb_recorder::table_name_for(layout.model);
        layout.variables.reserve(storage->variables.size());

        for (const auto &variable : storage->variables)
        {
            DuckDbVariableLayout variable_layout;
            variable_layout.name = duckdb_recorder::local_variable_name(layout.model, variable.name);
            variable_layout.type = variable.type;
            variable_layout.position = variable.position;
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

    void DuckDbRecorderSink::open_layout(DuckDbStorageLayout &layout)
    {
        std::string create_sql = "CREATE TABLE IF NOT EXISTS ";
        create_sql += duckdb_recorder::quote_identifier(layout.table_name);
        create_sql += " (";
        create_sql += duckdb_recorder::quote_identifier("timestamp_ns");
        create_sql += " BIGINT, ";
        create_sql += duckdb_recorder::quote_identifier("simulation_time_s");
        create_sql += " DOUBLE";

        for (const auto &variable : layout.variables)
        {
            create_sql += ", ";
            create_sql += duckdb_recorder::quote_identifier(variable.name);
            create_sql += ' ';
            create_sql += duckdb_recorder::sql_type_for(variable.type);
        }

        create_sql += ");";

        duckdb_recorder::execute_query(connection, create_sql, "Failed to create DuckDB table");
        duckdb_recorder::insert_metadata_row(connection, layout);

        duckdb_recorder::check_duckdb_state(duckdb_appender_create(connection, nullptr, layout.table_name.c_str(), &layout.appender), "Failed to create DuckDB appender");
    }

    void DuckDbRecorderSink::init()
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

    void DuckDbRecorderSink::on_event(const NewDataEvent &event)
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
        if (layout.appender == nullptr)
        {
            LOG_WARNING(log, "[{func}] Ignoring event for unopened storage {}", __func__, event.storage->name);
            return;
        }

        const auto simulation_time_s = utils::time::ns_to_s(event.timestamp);
        const auto timestamp_ns = static_cast<std::int64_t>(event.timestamp);

        try
        {
            duckdb_recorder::check_duckdb_state(duckdb_append_int64(layout.appender, timestamp_ns), "Failed to append timestamp");
            duckdb_recorder::check_duckdb_state(duckdb_append_double(layout.appender, simulation_time_s), "Failed to append simulation time");

            for (const auto &variable : layout.variables)
            {
                duckdb_recorder::append_value(layout.appender, variable.type, event.buffer + variable.position);
            }

            duckdb_recorder::check_duckdb_state(duckdb_appender_end_row(layout.appender), "Failed to finish DuckDB row");
        }
        catch (const std::exception &e)
        {
            disable_sink(e.what());
        }
    }

    void DuckDbRecorderSink::disable_sink(const std::string &reason)
    {
        if (disabled)
        {
            return;
        }

        disabled = true;
        stopped = true;
        LOG_WARNING(log, "[{func}] DuckDB sink disabled: {}", __func__, reason);

        for (auto &layout : layouts)
        {
            if (layout.appender != nullptr)
            {
                duckdb_appender_destroy(&layout.appender);
            }
        }

        if (connection != nullptr)
        {
            duckdb_disconnect(&connection);
            connection = nullptr;
        }

        if (database != nullptr)
        {
            duckdb_close(&database);
            database = nullptr;
        }
    }

    void DuckDbRecorderSink::stop()
    {
        if (disabled || stopped)
        {
            return;
        }

        try
        {
            for (auto &layout : layouts)
            {
                if (layout.appender != nullptr)
                {
                    duckdb_recorder::check_duckdb_state(duckdb_appender_destroy(&layout.appender), "Failed to destroy DuckDB appender");
                }
            }
        }
        catch (const std::exception &e)
        {
            disable_sink(e.what());
            return;
        }

        if (connection != nullptr)
        {
            duckdb_disconnect(&connection);
            connection = nullptr;
        }

        if (database != nullptr)
        {
            duckdb_close(&database);
            database = nullptr;
        }

        stopped = true;
    }
}
