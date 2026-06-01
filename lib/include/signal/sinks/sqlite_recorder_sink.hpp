#pragma once

#include "signal/recorder.hpp"
#include "signal/sinks/sqlite_recorder_storage.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <sqlite3.h>

#include <cstdint>
#include <filesystem>
#include <optional>
#include <unordered_map>
#include <vector>

namespace ssp4sim::signal
{
    class SqliteWALRecorderSink final : public RecorderSink
    {
    public:
        ssp4cpp::utils::log::Logger *log = nullptr;

        std::filesystem::path working_dir;
        std::string session_uuid;
        std::optional<std::filesystem::path> file_override;
        std::filesystem::path filename;
        int64_t run_id = 0;
        sqlite3 *db = nullptr;
        std::vector<SqliteStorageLayout> layouts;
        std::unordered_map<const SignalStorage *, std::size_t> layout_lookup;

        bool disabled = false;
        bool initialized = false;
        bool stopped = false;

        SqliteWALRecorderSink(std::filesystem::path working_dir, std::string session_uuid, std::optional<std::filesystem::path> file_override);
        ~SqliteWALRecorderSink() override;

        void on_storage_added(const SignalStorage *storage) override;

        void init() override;

        void on_event(const NewDataEvent &event) override;

        void stop() override;

    private:
        static constexpr std::size_t commit_interval = 10'000;
        std::size_t insert_count = 0;

        void open_database();

        void execute_pragma(const char *sql, const char *context);

        void begin_transaction();

        void commit_transaction(const char *context);

        void open_layout(SqliteStorageLayout &layout);

        void disable_sink(const std::string &reason);

        static std::string local_variable_name(const std::string &storage_model, const std::string &name);
    };
}
