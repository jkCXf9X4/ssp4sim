#pragma once

#include "signal/sinks/sqlite_recorder_storage.hpp"

#include <sqlite3.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

namespace ssp4sim::signal::sqlite_recorder
{
    void check_sqlite_error(int rc, std::string_view context);

    std::string quote_identifier(const std::string &name);

    std::pair<std::string, std::string> split_storage_name(const std::string &name);

    std::string table_name_for(const std::string &model);

    void create_metadata_table(sqlite3 *db);

    void insert_metadata_row(sqlite3 *db, const SqliteStorageLayout &layout);

    std::string sql_type_for(types::DataType type);
}