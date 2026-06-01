#pragma once

#include "signal/recorder.hpp"

#include <sqlite3.h>

#include <cstddef>
#include <string>
#include <vector>

namespace ssp4sim::signal
{
    struct SqliteVariableLayout
    {
        std::string name;
        types::DataType type;
        std::size_t position = 0;
    };

    struct SqliteStorageLayout
    {
        const SignalStorage *storage = nullptr;
        std::size_t index = 0;
        std::string model;
        std::string storage_name;
        std::string table_name;
        std::vector<SqliteVariableLayout> variables;
        sqlite3_stmt *insert_stmt = nullptr;
    };
}