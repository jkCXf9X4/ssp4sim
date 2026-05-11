#include "signal/sinks/duckdb_recorder_utils.hpp"

#include <array>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <format>
#include <random>
#include <stdexcept>

namespace ssp4sim::signal::duckdb_recorder
{
    namespace
    {
        constexpr std::string_view metadata_table_name = "ssp4sim_metadata";

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

        std::string uuid_suffix()
        {
            std::random_device random;
            std::array<std::uint32_t, 4> parts{};
            for (auto &part : parts)
            {
                part = random();
            }

            return std::format("{:08x}{:08x}{:08x}{:08x}", parts[0], parts[1], parts[2], parts[3]);
        }

        std::int64_t current_epoch_seconds()
        {
            using namespace std::chrono;
            const auto now = system_clock::now();
            return duration_cast<seconds>(now.time_since_epoch()).count();
        }

        std::string duckdb_error_message(duckdb_result &result, std::string_view context)
        {
            std::string message(context);
            const auto *error = duckdb_result_error(&result);
            if (error != nullptr && *error != '\0')
            {
                message += ": ";
                message += error;
            }

            return message;
        }

        void bind_varchar(duckdb_prepared_statement statement, idx_t index, const std::string &value, std::string_view context)
        {
            check_duckdb_state(duckdb_bind_varchar(statement, index, value.c_str()), context);
        }
    }

    void check_duckdb_state(duckdb_state state, std::string_view context, std::string_view error)
    {
        if (state != DuckDBSuccess)
        {
            if (!error.empty())
            {
                std::string message(context);
                message += ": ";
                message += error;
                throw std::runtime_error(message);
            }

            throw std::runtime_error(std::string(context));
        }
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

    std::pair<std::string, std::string> split_storage_name(const std::string &name)
    {
        const auto separator = name.find('.');
        if (separator == std::string::npos)
        {
            return {"", name};
        }

        return {name.substr(0, separator), name.substr(separator + 1)};
    }

    std::string local_variable_name(const std::string &storage_model, const std::string &name)
    {
        const auto prefix = storage_model.empty() ? std::string{} : storage_model + '.';
        if (!prefix.empty() && name.rfind(prefix, 0) == 0 && name.size() > prefix.size())
        {
            return name.substr(prefix.size());
        }

        return name;
    }

    std::string table_name_for(const std::string &model)
    {
        return std::format("{}_{}_{}", sanitize_component(model), current_epoch_seconds(), uuid_suffix());
    }

    void execute_query(duckdb_connection connection, const std::string &sql, std::string_view context)
    {
        duckdb_result result;
        const auto state = duckdb_query(connection, sql.c_str(), &result);
        if (state != DuckDBSuccess)
        {
            const auto message = duckdb_error_message(result, context);
            duckdb_destroy_result(&result);
            throw std::runtime_error(message);
        }

        duckdb_destroy_result(&result);
    }

    void create_metadata_table(duckdb_connection connection)
    {
        std::string sql = "CREATE TABLE IF NOT EXISTS ";
        sql += quote_identifier(std::string(metadata_table_name));
        sql += " (";
        sql += quote_identifier("table_name");
        sql += " VARCHAR PRIMARY KEY, ";
        sql += quote_identifier("model");
        sql += " VARCHAR, ";
        sql += quote_identifier("storage_name");
        sql += " VARCHAR, ";
        sql += quote_identifier("source_storage_name");
        sql += " VARCHAR, ";
        sql += quote_identifier("created_at_s");
        sql += " BIGINT";
        sql += ");";

        execute_query(connection, sql, "Failed to create DuckDB metadata table");
    }

    void insert_metadata_row(duckdb_connection connection, const DuckDbStorageLayout &layout)
    {
        const std::string sql = std::format(
            "INSERT INTO {} ({}, {}, {}, {}, {}) VALUES (?, ?, ?, ?, ?);",
            quote_identifier(std::string(metadata_table_name)),
            quote_identifier("table_name"),
            quote_identifier("model"),
            quote_identifier("storage_name"),
            quote_identifier("source_storage_name"),
            quote_identifier("created_at_s"));

        duckdb_prepared_statement statement = nullptr;
        check_duckdb_state(duckdb_prepare(connection, sql.c_str(), &statement), "Failed to prepare DuckDB metadata insert");

        try
        {
            bind_varchar(statement, 1, layout.table_name, "Failed to bind DuckDB metadata table name");
            bind_varchar(statement, 2, layout.model, "Failed to bind DuckDB metadata model");
            bind_varchar(statement, 3, layout.storage_name, "Failed to bind DuckDB metadata storage name");
            bind_varchar(statement, 4, layout.storage->name, "Failed to bind DuckDB metadata source storage name");
            check_duckdb_state(duckdb_bind_int64(statement, 5, current_epoch_seconds()), "Failed to bind DuckDB metadata creation time");

            duckdb_result result;
            const auto state = duckdb_execute_prepared(statement, &result);
            if (state != DuckDBSuccess)
            {
                const auto message = duckdb_error_message(result, "Failed to insert DuckDB metadata row");
                duckdb_destroy_result(&result);
                throw std::runtime_error(message);
            }
            duckdb_destroy_result(&result);
        }
        catch (...)
        {
            duckdb_destroy_prepare(&statement);
            throw;
        }

        duckdb_destroy_prepare(&statement);
    }

    std::string sql_type_for(types::DataType type)
    {
        switch (static_cast<types::DataType::Value>(type))
        {
        case types::DataType::Value::real:
            return "DOUBLE";
        case types::DataType::Value::integer:
        case types::DataType::Value::enumeration:
            return "INTEGER";
        case types::DataType::Value::boolean:
            return "BOOLEAN";
        case types::DataType::Value::string:
            return "VARCHAR";
        default:
            throw std::runtime_error("Unsupported data type for DuckDB recording");
        }
    }

    void append_value(duckdb_appender appender, types::DataType type, const std::byte *data)
    {
        switch (static_cast<types::DataType::Value>(type))
        {
        case types::DataType::Value::real:
            check_duckdb_state(duckdb_append_double(appender, *reinterpret_cast<const double *>(data)), "Failed to append floating-point value");
            break;
        case types::DataType::Value::integer:
        case types::DataType::Value::enumeration:
            check_duckdb_state(duckdb_append_int32(appender, *reinterpret_cast<const int *>(data)), "Failed to append integer value");
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
}
