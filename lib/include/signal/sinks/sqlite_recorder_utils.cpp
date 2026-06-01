#include "signal/sinks/sqlite_recorder_utils.hpp"

#include <array>
#include <chrono>
#include <cctype>
#include <cstdint>
#include <format>
#include <random>
#include <stdexcept>
#include <string>

namespace ssp4sim::signal::sqlite_recorder
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

        void bind_text(sqlite3_stmt *stmt, int index, const std::string &value, std::string_view context)
        {
            const auto rc = sqlite3_bind_text(stmt, index, value.c_str(), static_cast<int>(value.size()), SQLITE_TRANSIENT);
            if (rc != SQLITE_OK)
            {
                throw std::runtime_error(std::string(context) + ": " + sqlite3_errmsg(sqlite3_db_handle(stmt)));
            }
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

    std::string table_name_for(const std::string &model)
    {
        return std::format("{}_{}_{}", sanitize_component(model), current_epoch_seconds(), uuid_suffix());
    }

    void create_metadata_table(sqlite3 *db)
    {
        std::string sql = "CREATE TABLE IF NOT EXISTS ";
        sql += quote_identifier(std::string(metadata_table_name));
        sql += " (";
        sql += quote_identifier("table_name");
        sql += " TEXT PRIMARY KEY, ";
        sql += quote_identifier("model");
        sql += " TEXT, ";
        sql += quote_identifier("storage_name");
        sql += " TEXT, ";
        sql += quote_identifier("source_storage_name");
        sql += " TEXT, ";
        sql += quote_identifier("created_at_s");
        sql += " INTEGER";
        sql += ");";

        exec_sql(db, sql, "Failed to create SQLite metadata table");
    }

    void insert_metadata_row(sqlite3 *db, const SqliteStorageLayout &layout)
    {
        const std::string sql = std::format(
            "INSERT INTO {} ({}, {}, {}, {}, {}) VALUES (?, ?, ?, ?, ?);",
            quote_identifier(std::string(metadata_table_name)),
            quote_identifier("table_name"),
            quote_identifier("model"),
            quote_identifier("storage_name"),
            quote_identifier("source_storage_name"),
            quote_identifier("created_at_s"));

        sqlite3_stmt *stmt = nullptr;
        auto rc = sqlite3_prepare_v2(db, sql.c_str(), static_cast<int>(sql.size()), &stmt, nullptr);
        if (rc != SQLITE_OK)
        {
            throw std::runtime_error("Failed to prepare SQLite metadata insert: " + std::string(sqlite3_errmsg(db)));
        }

        try
        {
            bind_text(stmt, 1, layout.table_name, "Failed to bind SQLite metadata table name");
            bind_text(stmt, 2, layout.model, "Failed to bind SQLite metadata model");
            bind_text(stmt, 3, layout.storage_name, "Failed to bind SQLite metadata storage name");
            bind_text(stmt, 4, layout.storage->name, "Failed to bind SQLite metadata source storage name");

            rc = sqlite3_bind_int64(stmt, 5, current_epoch_seconds());
            if (rc != SQLITE_OK)
            {
                throw std::runtime_error("Failed to bind SQLite metadata creation time: " + std::string(sqlite3_errmsg(db)));
            }

            rc = sqlite3_step(stmt);
            if (rc != SQLITE_DONE)
            {
                throw std::runtime_error("Failed to insert SQLite metadata row: " + std::string(sqlite3_errmsg(db)));
            }
        }
        catch (...)
        {
            sqlite3_finalize(stmt);
            throw;
        }

        sqlite3_finalize(stmt);
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