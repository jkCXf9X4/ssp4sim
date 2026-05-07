#pragma once

#include "signal/recorder.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <arrow/api.h>
#include <arrow/io/api.h>
#include <parquet/arrow/writer.h>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace ssp4sim::signal
{
    struct ParquetVariableLayout
    {
        std::string name;
        types::DataType type;
        std::size_t position = 0;
        std::size_t column = 0;
    };

    struct ParquetStorageLayout
    {
        const SignalStorage *storage = nullptr;
        std::size_t index = 0;
        std::string model;
        std::string storage_name;
        std::vector<ParquetVariableLayout> variables;
    };

    struct ParquetColumnLayout
    {
        std::string name;
        types::DataType type;
    };

    class ParquetRecorderSink final : public RecorderSink
    {
    public:
        ssp4cpp::utils::log::Logger *log = nullptr;

        std::filesystem::path filename;
        std::shared_ptr<arrow::Schema> schema;
        std::shared_ptr<arrow::io::FileOutputStream> output;
        std::unique_ptr<parquet::arrow::FileWriter> writer;

        std::vector<ParquetStorageLayout> layouts;
        std::unordered_map<const SignalStorage *, std::size_t> layout_lookup;
        std::unordered_map<std::string, std::size_t> column_lookup;
        std::vector<ParquetColumnLayout> columns;
        std::vector<std::unique_ptr<arrow::ArrayBuilder>> builders;

        std::size_t batch_rows = 1024;
        std::size_t row_count = 0;
        bool disabled = false;

        explicit ParquetRecorderSink(const std::filesystem::path &filename);

        void on_storage_added(const SignalStorage *storage) override;

        void init() override;

        void on_event(const NewDataEvent &event) override;

        void stop() override;

    private:
        static std::pair<std::string, std::string> split_storage_name(const std::string &name);

        static std::string local_variable_name(const std::string &storage_model, const std::string &name);

        static std::shared_ptr<arrow::DataType> arrow_type_for(types::DataType type);

        static std::unique_ptr<arrow::ArrayBuilder> make_builder(const std::shared_ptr<arrow::DataType> &type);

        static void append_typed_value(arrow::ArrayBuilder &builder, types::DataType type, const std::byte *data);

        void rebuild_builders();

        void flush_batch();

        void disable_sink(const std::string &reason);
    };
}
