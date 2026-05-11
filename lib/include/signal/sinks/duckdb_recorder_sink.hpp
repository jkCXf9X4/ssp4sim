#pragma once

#include "signal/recorder.hpp"
#include "signal/sinks/duckdb_recorder_storage.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <duckdb.h>

#include <filesystem>
#include <unordered_map>
#include <vector>

namespace ssp4sim::signal
{
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

        void open_database();

        void open_layout(DuckDbStorageLayout &layout);

        void disable_sink(const std::string &reason);
    };
}
