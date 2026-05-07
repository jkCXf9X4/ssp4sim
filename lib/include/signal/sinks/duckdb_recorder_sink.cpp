#include "signal/sinks/duckdb_recorder_sink.hpp"

#include "utils/io.hpp"
#include "utils/time.hpp"

#include <duckdb.h>

#include <algorithm>
#include <cctype>
#include <stdexcept>
#include <string_view>

namespace ssp4sim::signal
{
    namespace
    {
        void check_duckdb_state(duckdb_state state, const std::string &context, const std::string *error = nullptr)
        {
            if (state != DuckDBSuccess)
            {
                if (error != nullptr && !error->empty())
                {
                    throw std::runtime_error(context + ": " + *error);
                }

                throw std::runtime_error(context);
            }
        }

        std::string append_sql_column_name(const std::string &name)
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
    }

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

    std::pair<std::string, std::string> DuckDbRecorderSink::split_storage_name(const std::string &name)
    {
        const auto separator = name.find('.');
        if (separator == std::string::npos)
        {
            return {"", name};
        }

        return {name.substr(0, separator), name.substr(separator + 1)};
    }

    std::string DuckDbRecorderSink::local_variable_name(const std::string &storage_model, const std::string &name)
    {
        const auto prefix = storage_model.empty() ? std::string{} : storage_model + '.';
        if (!prefix.empty() && name.rfind(prefix, 0) == 0 && name.size() > prefix.size())
        {
            return name.substr(prefix.size());
        }

        return name;
    }

    std::string DuckDbRecorderSink::sanitize_component(std::string_view value)
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

    std::string DuckDbRecorderSink::table_name_for(std::size_t index, const std::string &model, const std::string &storage_name)
    {
        std::string table_name = "duckdb_";
        table_name += std::to_string(index);
        if (!model.empty())
        {
            table_name += '_';
            table_name += sanitize_component(model);
        }
        if (!storage_name.empty())
        {
            table_name += '_';
            table_name += sanitize_component(storage_name);
        }

        return table_name;
    }

    std::string DuckDbRecorderSink::sql_type_for(types::DataType type)
    {
        switch (static_cast<types::DataType::Value>(type))
        {
        case types::DataType::Value::real:
            return "DOUBLE";
        case types::DataType::Value::integer:
        case types::DataType::Value::enumeration:
            return "BIGINT";
        case types::DataType::Value::boolean:
            return "BOOLEAN";
        case types::DataType::Value::string:
            return "VARCHAR";
        default:
            throw std::runtime_error("Unsupported data type for DuckDB recording");
        }
    }

    std::string DuckDbRecorderSink::quote_identifier(const std::string &name)
    {
        return append_sql_column_name(name);
    }

    void DuckDbRecorderSink::append_value(duckdb_appender appender, types::DataType type, const std::byte *data)
    {
        switch (static_cast<types::DataType::Value>(type))
        {
        case types::DataType::Value::real:
            check_duckdb_state(duckdb_append_double(appender, *reinterpret_cast<const double *>(data)), "Failed to append floating-point value");
            break;
        case types::DataType::Value::integer:
        case types::DataType::Value::enumeration:
            check_duckdb_state(duckdb_append_int64(appender, static_cast<int64_t>(*reinterpret_cast<const int *>(data))), "Failed to append integer value");
            break;
        case types::DataType::Value::boolean:
            check_duckdb_state(duckdb_append_bool(appender, *reinterpret_cast<const int *>(data) != 0), "Failed to append boolean value");
            break;
        case types::DataType::Value::string:
            check_duckdb_state(duckdb_append_varchar(appender, reinterpret_cast<const std::string *>(data)->c_str()), "Failed to append string value");
            break;
        default:
            throw std::runtime_error("Unsupported data type for DuckDB recording");
        }
    }

    void DuckDbRecorderSink::open_database()
    {
        utils::io::create_parent_folder(filename.string());
        if (std::filesystem::exists(filename))
        {
            std::filesystem::remove(filename);
        }

        check_duckdb_state(duckdb_open(filename.string().c_str(), &database), "Failed to open DuckDB database");
        check_duckdb_state(duckdb_connect(database, &connection), "Failed to connect to DuckDB database");
    }

    void DuckDbRecorderSink::on_storage_added(const SignalStorage *storage)
    {
        if (storage == nullptr || storage->mem_size == 0)
        {
            return;
        }

        auto [model, storage_name] = split_storage_name(storage->name);

        DuckDbStorageLayout layout;
        layout.storage = storage;
        layout.index = layouts.size();
        layout.model = std::move(model);
        layout.storage_name = std::move(storage_name);
        layout.table_name = table_name_for(layout.index, layout.model, layout.storage_name);
        layout.variables.reserve(storage->variables.size());

        for (const auto &variable : storage->variables)
        {
            DuckDbVariableLayout variable_layout;
            variable_layout.name = local_variable_name(layout.model, variable.name);
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
        create_sql += quote_identifier(layout.table_name);
        create_sql += " (";
        create_sql += quote_identifier("timestamp_ns");
        create_sql += " BIGINT, ";
        create_sql += quote_identifier("simulation_time_s");
        create_sql += " DOUBLE, ";
        create_sql += quote_identifier("model");
        create_sql += " VARCHAR, ";
        create_sql += quote_identifier("storage");
        create_sql += " VARCHAR";

        for (const auto &variable : layout.variables)
        {
            create_sql += ", ";
            create_sql += quote_identifier(variable.name);
            create_sql += ' ';
            create_sql += sql_type_for(variable.type);
        }

        create_sql += ");";

        duckdb_result result;
        const auto state = duckdb_query(connection, create_sql.c_str(), &result);
        if (state != DuckDBSuccess)
        {
            const auto *error = duckdb_result_error(&result);
            std::string message = "Failed to create DuckDB table";
            if (error != nullptr && *error != '\0')
            {
                message += ": ";
                message += error;
            }
            duckdb_destroy_result(&result);
            throw std::runtime_error(message);
        }
        duckdb_destroy_result(&result);

        check_duckdb_state(duckdb_appender_create(connection, nullptr, layout.table_name.c_str(), &layout.appender), "Failed to create DuckDB appender");
    }

    void DuckDbRecorderSink::init()
    {
        LOG_TRACE_L1(log, "[{func}] Init", __func__);
        if (disabled)
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

    void DuckDbRecorderSink::flush_batch(DuckDbStorageLayout &layout)
    {
        if (disabled || layout.appender == nullptr || layout.row_count == 0)
        {
            return;
        }

        try
        {
            check_duckdb_state(duckdb_appender_flush(layout.appender), "Failed to flush DuckDB appender");
            layout.row_count = 0;
        }
        catch (const std::exception &e)
        {
            disable_sink(e.what());
        }
    }

    void DuckDbRecorderSink::on_event(const NewDataEvent &event)
    {
        if (disabled || event.storage == nullptr || event.buffer == nullptr)
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
            check_duckdb_state(duckdb_append_int64(layout.appender, timestamp_ns), "Failed to append timestamp");
            check_duckdb_state(duckdb_append_double(layout.appender, simulation_time_s), "Failed to append simulation time");
            check_duckdb_state(duckdb_append_varchar(layout.appender, layout.model.c_str()), "Failed to append model");
            check_duckdb_state(duckdb_append_varchar(layout.appender, layout.storage_name.c_str()), "Failed to append storage");

            for (const auto &variable : layout.variables)
            {
                append_value(layout.appender, variable.type, event.buffer + variable.position);
            }

            check_duckdb_state(duckdb_appender_end_row(layout.appender), "Failed to finish DuckDB row");

            layout.row_count += 1;
            if (layout.row_count >= batch_rows)
            {
                flush_batch(layout);
            }
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
        if (disabled)
        {
            return;
        }

        try
        {
            for (auto &layout : layouts)
            {
                flush_batch(layout);
                if (layout.appender != nullptr)
                {
                    check_duckdb_state(duckdb_appender_destroy(&layout.appender), "Failed to destroy DuckDB appender");
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
    }
}
