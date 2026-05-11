#pragma once

#include "signal/sinks/duckdb_recorder_storage.hpp"

#include <duckdb.h>

#include <cstddef>
#include <string>
#include <string_view>
#include <utility>

namespace ssp4sim::signal::duckdb_recorder
{
    void check_duckdb_state(duckdb_state state, std::string_view context, std::string_view error = {});

    std::string quote_identifier(const std::string &name);

    std::pair<std::string, std::string> split_storage_name(const std::string &name);

    std::string local_variable_name(const std::string &storage_model, const std::string &name);

    std::string table_name_for(const std::string &model);

    void execute_query(duckdb_connection connection, const std::string &sql, std::string_view context);

    void create_metadata_table(duckdb_connection connection);

    void insert_metadata_row(duckdb_connection connection, const DuckDbStorageLayout &layout);

    std::string sql_type_for(types::DataType type);

    void append_value(duckdb_appender appender, types::DataType type, const std::byte *data);
}
