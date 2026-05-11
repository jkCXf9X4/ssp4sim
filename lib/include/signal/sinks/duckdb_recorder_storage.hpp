#pragma once

#include "signal/recorder.hpp"

#include <duckdb.h>

#include <cstddef>
#include <string>
#include <vector>

namespace ssp4sim::signal
{
    struct DuckDbVariableLayout
    {
        std::string name;
        types::DataType type;
        std::size_t position = 0;
    };

    struct DuckDbStorageLayout
    {
        const SignalStorage *storage = nullptr;
        std::size_t index = 0;
        std::string model;
        std::string storage_name;
        std::string table_name;
        std::vector<DuckDbVariableLayout> variables;
        duckdb_appender appender = nullptr;
    };
}
