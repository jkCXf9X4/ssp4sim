#pragma once

#include "signal/recorder.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <duckdb.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
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
        std::string table_name;
        std::vector<DuckDbVariableLayout> variables;
        duckdb_appender appender = nullptr;
    };

    class DuckDbRecorderSink final : public RecorderSink
    {
    public:
        ssp4cpp::utils::log::Logger *log = nullptr;

        std::filesystem::path filename;
        duckdb_database database = nullptr;
        duckdb_connection connection = nullptr;
        std::vector<DuckDbStorageLayout> layouts;
        std::unordered_map<const SignalStorage *, std::size_t> layout_lookup;

        bool disabled = false;
        bool initialized = false;
        bool stopped = false;

        explicit DuckDbRecorderSink(const std::filesystem::path &filename);
        ~DuckDbRecorderSink() override;

        void on_storage_added(const SignalStorage *storage) override;

        void init() override;

        void on_event(const NewDataEvent &event) override;

        void stop() override;

    private:
        static std::pair<std::string, std::string> split_storage_name(const std::string &name);

        static std::string local_variable_name(const std::string &storage_model, const std::string &name);

        static std::string sanitize_component(std::string_view value);

        static std::string table_name_for(std::size_t index, const std::string &model, const std::string &storage_name);

        static std::string sql_type_for(types::DataType type);

        static std::string quote_identifier(const std::string &name);

        static void append_value(duckdb_appender appender, types::DataType type, const std::byte *data);

        void open_database();

        void open_layout(DuckDbStorageLayout &layout);

        void disable_sink(const std::string &reason);
    };
}
