#pragma once

#include "ssp4sim_definitions.hpp"

#include <sqlite3.h>

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace ssp4sim::signal::sqlite_recorder
{
    void check_sqlite_error(int rc, std::string_view context);

    std::string quote_identifier(const std::string &name);

    std::pair<std::string, std::string> split_storage_name(const std::string &name);

    std::string table_name_for(int64_t run_id, const std::string &model, const std::string &storage_name);

    int64_t run_counter(sqlite3 *db);

    std::string sql_type_for(types::DataType type);
}