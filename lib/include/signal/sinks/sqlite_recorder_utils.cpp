#include "signal/sinks/sqlite_recorder_utils.hpp"

#include <cctype>
#include <cstdint>
#include <format>
#include <stdexcept>
#include <string>

namespace ssp4sim::signal::sqlite_recorder
{
    namespace
    {
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

        void exec_sql(sqlite3 *db, const std::string &sql, std::string_view context)
        {
            char *error_message = nullptr;
            const auto rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &error_message);
            if (rc != SQLITE_OK)
            {
                const std::string msg = std::string(context) + ": " + error_message;
                sqlite3_free(error_message);
                throw std::runtime_error(msg);
            }
        }
    }

    void check_sqlite_error(int rc, std::string_view context)
    {
        if (rc != SQLITE_OK)
        {
            throw std::runtime_error(std::string(context) + " (rc=" + std::to_string(rc) + ")");
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

    std::string table_name_for(int64_t run_id, const std::string &model, const std::string &storage_name)
    {
        return std::format("_{}_{}_{}", run_id, sanitize_component(model), sanitize_component(storage_name));
    }

    int64_t run_counter(sqlite3 *db)
    {
        // Atomic read-increment-store using BEGIN IMMEDIATE
        exec_sql(db, "BEGIN IMMEDIATE;", "Failed to begin run_counter transaction");

        // Read current max run_id
        std::int64_t current_id = 0;
        {
            const std::string sql = "SELECT COALESCE(MAX(run_id), 0) FROM ssp4sim_run_counter;";
            sqlite3_stmt *stmt = nullptr;
            auto rc = sqlite3_prepare_v2(db, sql.c_str(), static_cast<int>(sql.size()), &stmt, nullptr);
            if (rc != SQLITE_OK)
            {
                sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
                throw std::runtime_error("Failed to prepare run_counter SELECT: " + std::string(sqlite3_errmsg(db)));
            }
            rc = sqlite3_step(stmt);
            if (rc == SQLITE_ROW)
            {
                current_id = sqlite3_column_int64(stmt, 0);
            }
            sqlite3_finalize(stmt);
        }

        const auto next_id = current_id + 1;

        // Insert new run_id
        {
            const std::string sql = "INSERT INTO ssp4sim_run_counter (run_id) VALUES (" + std::to_string(next_id) + ");";
            exec_sql(db, sql, "Failed to insert run_counter row");
        }

        exec_sql(db, "COMMIT;", "Failed to commit run_counter transaction");

        return next_id;
    }

    std::string sql_type_for(types::DataType type)
    {
        switch (static_cast<types::DataType::Value>(type))
        {
        case types::DataType::Value::real:
            return "REAL";
        case types::DataType::Value::integer:
        case types::DataType::Value::enumeration:
        case types::DataType::Value::boolean:
            return "INTEGER";
        case types::DataType::Value::string:
            return "TEXT";
        default:
            throw std::runtime_error("Unsupported data type for SQLite recording");
        }
    }
}