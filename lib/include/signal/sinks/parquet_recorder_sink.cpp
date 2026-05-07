#include "signal/sinks/parquet_recorder_sink.hpp"

#include "utils/io.hpp"
#include "utils/time.hpp"

#include <arrow/io/api.h>
#include <arrow/result.h>
#include <parquet/properties.h>

#include <cctype>
#include <algorithm>
#include <chrono>
#include <stdexcept>
#include <string_view>

namespace ssp4sim::signal
{
    namespace
    {
        void check_status(const arrow::Status &status, const std::string &context)
        {
            if (!status.ok())
            {
                throw std::runtime_error(context + ": " + status.ToString());
            }
        }

        template <typename T>
        T *builder_as(arrow::ArrayBuilder &builder)
        {
            auto *typed = dynamic_cast<T *>(&builder);
            if (typed == nullptr)
            {
                throw std::runtime_error("Parquet builder type mismatch");
            }
            return typed;
        }

        std::shared_ptr<arrow::Field> make_field(const std::string &name, const std::shared_ptr<arrow::DataType> &type)
        {
            return arrow::field(name, type);
        }

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

        std::filesystem::path make_storage_file_path(const std::filesystem::path &base, const std::string &model, const std::string &storage_name)
        {
            auto filename = base.stem().string();
            if (!model.empty())
            {
                filename += '_' + sanitize_component(model);
            }
            if (!storage_name.empty())
            {
                filename += '_' + sanitize_component(storage_name);
            }

            const auto extension = base.extension().empty() ? std::filesystem::path(".parquet").string() : base.extension().string();
            filename += extension;
            return base.parent_path() / filename;
        }
    }

    ParquetRecorderSink::ParquetRecorderSink(const std::filesystem::path &filename)
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.signal.ParquetRecorderSink")),
          filename(filename)
    {
        LOG_DEBUG(log, "[{func}] File {file}", __func__, filename.string());
    }

    std::pair<std::string, std::string> ParquetRecorderSink::split_storage_name(const std::string &name)
    {
        const auto separator = name.find('.');
        if (separator == std::string::npos)
        {
            return {"", name};
        }

        return {name.substr(0, separator), name.substr(separator + 1)};
    }

    std::string ParquetRecorderSink::local_variable_name(const std::string &storage_model, const std::string &name)
    {
        const auto prefix = storage_model.empty() ? std::string{} : storage_model + '.';
        if (!prefix.empty() && name.rfind(prefix, 0) == 0 && name.size() > prefix.size())
        {
            return name.substr(prefix.size());
        }

        return name;
    }

    std::shared_ptr<arrow::DataType> ParquetRecorderSink::arrow_type_for(types::DataType type)
    {
        switch (static_cast<types::DataType::Value>(type))
        {
        case types::DataType::Value::real:
            return arrow::float64();
        case types::DataType::Value::integer:
        case types::DataType::Value::enumeration:
            return arrow::int64();
        case types::DataType::Value::boolean:
            return arrow::boolean();
        case types::DataType::Value::string:
            return arrow::utf8();
        default:
            throw std::runtime_error("Unsupported data type for Parquet recording");
        }
    }

    std::filesystem::path ParquetRecorderSink::storage_file_path(const std::filesystem::path &base, const std::string &model, const std::string &storage_name)
    {
        return make_storage_file_path(base, model, storage_name);
    }

    void ParquetRecorderSink::reserve_builder_capacity(arrow::ArrayBuilder &builder, std::size_t rows)
    {
        check_status(builder.Reserve(static_cast<int64_t>(rows)), "Failed to reserve Parquet builder capacity");
    }

    std::unique_ptr<arrow::ArrayBuilder> ParquetRecorderSink::make_builder(const std::shared_ptr<arrow::DataType> &type)
    {
        switch (type->id())
        {
        case arrow::Type::DOUBLE:
            return std::make_unique<arrow::DoubleBuilder>(arrow::default_memory_pool());
        case arrow::Type::INT64:
            return std::make_unique<arrow::Int64Builder>(arrow::default_memory_pool());
        case arrow::Type::BOOL:
            return std::make_unique<arrow::BooleanBuilder>(arrow::default_memory_pool());
        case arrow::Type::STRING:
            return std::make_unique<arrow::StringBuilder>(arrow::default_memory_pool());
        default:
            throw std::runtime_error("Unsupported Arrow builder type for Parquet recording");
        }
    }

    void ParquetRecorderSink::append_typed_value(arrow::ArrayBuilder &builder, types::DataType type, const std::byte *data)
    {
        switch (static_cast<types::DataType::Value>(type))
        {
        case types::DataType::Value::real:
            check_status(builder_as<arrow::DoubleBuilder>(builder)->Append(*reinterpret_cast<const double *>(data)), "Failed to append floating-point value");
            break;
        case types::DataType::Value::integer:
        case types::DataType::Value::enumeration:
            check_status(builder_as<arrow::Int64Builder>(builder)->Append(static_cast<std::int64_t>(*reinterpret_cast<const int *>(data))), "Failed to append integer value");
            break;
        case types::DataType::Value::boolean:
            check_status(builder_as<arrow::BooleanBuilder>(builder)->Append(*reinterpret_cast<const int *>(data) != 0), "Failed to append boolean value");
            break;
        case types::DataType::Value::string:
            check_status(builder_as<arrow::StringBuilder>(builder)->Append(*reinterpret_cast<const std::string *>(data)), "Failed to append string value");
            break;
        default:
            throw std::runtime_error("Unsupported data type for Parquet recording");
        }
    }

    void ParquetRecorderSink::on_storage_added(const SignalStorage *storage)
    {
        if (storage == nullptr || storage->mem_size == 0)
        {
            return;
        }

        auto [model, storage_name] = split_storage_name(storage->name);

        ParquetStorageLayout layout;
        layout.storage = storage;
        layout.index = layouts.size();
        layout.model = std::move(model);
        layout.storage_name = std::move(storage_name);
        layout.variables.reserve(storage->variables.size());

        for (const auto &variable : storage->variables)
        {
            const auto local_name = local_variable_name(layout.model, variable.name);
            ParquetVariableLayout variable_layout;
            variable_layout.name = local_name;
            variable_layout.type = variable.type;
            variable_layout.position = variable.position;
            variable_layout.column = layout.variables.size();
            layout.variables.emplace_back(std::move(variable_layout));
        }

        layout.file = storage_file_path(filename, layout.model, layout.storage_name);
        layout_lookup[storage] = layout.index;
        layouts.emplace_back(std::move(layout));
    }

    void ParquetRecorderSink::rebuild_builders(ParquetStorageLayout &layout)
    {
        layout.builders.clear();
        layout.builders.reserve(4 + layout.variables.size());

        layout.builders.emplace_back(make_builder(arrow::int64()));
        layout.builders.emplace_back(make_builder(arrow::float64()));
        layout.builders.emplace_back(make_builder(arrow::utf8()));
        layout.builders.emplace_back(make_builder(arrow::utf8()));

        for (const auto &variable : layout.variables)
        {
            layout.builders.emplace_back(make_builder(arrow_type_for(variable.type)));
        }

        for (auto &builder : layout.builders)
        {
            reserve_builder_capacity(*builder, batch_rows);
        }
    }

    void ParquetRecorderSink::open_layout(ParquetStorageLayout &layout)
    {
        utils::io::create_parent_folder(layout.file.string());

        std::vector<std::shared_ptr<arrow::Field>> fields;
        fields.reserve(4 + layout.variables.size());
        fields.emplace_back(make_field("timestamp_ns", arrow::int64()));
        fields.emplace_back(make_field("simulation_time_s", arrow::float64()));
        fields.emplace_back(make_field("model", arrow::utf8()));
        fields.emplace_back(make_field("storage", arrow::utf8()));

        for (const auto &variable : layout.variables)
        {
            fields.emplace_back(make_field(variable.name, arrow_type_for(variable.type)));
        }

        layout.schema = arrow::schema(std::move(fields));
        rebuild_builders(layout);

        auto output_result = arrow::io::FileOutputStream::Open(layout.file.string());
        if (!output_result.ok())
        {
            throw std::runtime_error("Failed to open Parquet output file: " + output_result.status().ToString());
        }
        layout.output = std::move(output_result).ValueOrDie();

        auto writer_props_builder = parquet::WriterProperties::Builder();
        writer_props_builder.compression(parquet::Compression::UNCOMPRESSED);
        auto writer_props = writer_props_builder.build();
        auto arrow_props = parquet::ArrowWriterProperties::Builder().store_schema()->build();
        auto writer_result = parquet::arrow::FileWriter::Open(*layout.schema, arrow::default_memory_pool(), layout.output, writer_props, arrow_props);
        if (!writer_result.ok())
        {
            throw std::runtime_error("Failed to create Parquet writer: " + writer_result.status().ToString());
        }
        layout.writer = std::move(writer_result).ValueOrDie();
    }

    void ParquetRecorderSink::init()
    {
        LOG_TRACE_L1(log, "[{func}] Init", __func__);
        if (disabled)
        {
            return;
        }

        try
        {
            utils::io::create_parent_folder(filename.string());
            for (auto &layout : layouts)
            {
                open_layout(layout);
            }
        }
        catch (const std::exception &e)
        {
            disable_sink(e.what());
        }
    }

    void ParquetRecorderSink::disable_sink(const std::string &reason)
    {
        if (disabled)
        {
            return;
        }

        disabled = true;
        LOG_WARNING(log, "[{func}] Parquet sink disabled: {}", __func__, reason);
        for (auto &layout : layouts)
        {
            layout.writer.reset();
            layout.output.reset();
            layout.schema.reset();
            layout.builders.clear();
            layout.row_count = 0;
        }
    }

    void ParquetRecorderSink::on_event(const NewDataEvent &event)
    {
        if (disabled || event.storage == nullptr || event.buffer == nullptr)
        {
            return;
        }

        auto layout_it = layout_lookup.find(event.storage);
        if (layout_it == layout_lookup.end())
        {
            LOG_WARNING(log, "[{func}] Ignoring event for unknown storage {}", __func__, event.storage->name);
            return;
        }

        auto &layout = layouts[layout_it->second];
        const auto simulation_time_s = utils::time::ns_to_s(event.timestamp);
        const auto timestamp_ns = static_cast<std::int64_t>(event.timestamp);

        try
        {
            check_status(builder_as<arrow::Int64Builder>(*layout.builders[0])->Append(timestamp_ns), "Failed to append timestamp");
            check_status(builder_as<arrow::DoubleBuilder>(*layout.builders[1])->Append(simulation_time_s), "Failed to append simulation time");
            check_status(builder_as<arrow::StringBuilder>(*layout.builders[2])->Append(layout.model), "Failed to append model");
            check_status(builder_as<arrow::StringBuilder>(*layout.builders[3])->Append(layout.storage_name), "Failed to append storage");

            for (const auto &variable : layout.variables)
            {
                append_typed_value(*layout.builders[4 + variable.column], variable.type, event.buffer + variable.position);
            }

            layout.row_count += 1;
            if (layout.row_count >= batch_rows)
            {
                flush_batch(layout);
            }
        }
        catch (const std::exception &e)
        {
            disable_sink(layout, e.what());
        }
    }

    void ParquetRecorderSink::disable_sink(ParquetStorageLayout &layout, const std::string &reason)
    {
        if (!disabled)
        {
            const auto storage_name = layout.storage != nullptr ? layout.storage->name : layout.file.string();
            LOG_WARNING(log, "[{func}] Parquet sink disabled for {}: {}", __func__, storage_name, reason);
        }

        disable_sink(reason);
    }

    void ParquetRecorderSink::flush_batch(ParquetStorageLayout &layout)
    {
        if (disabled || layout.writer == nullptr || layout.row_count == 0)
        {
            return;
        }

        try
        {
            std::vector<std::shared_ptr<arrow::Array>> arrays;
            arrays.reserve(layout.builders.size());

            for (auto &builder : layout.builders)
            {
                std::shared_ptr<arrow::Array> array;
                auto status = builder->Finish(&array);
                if (!status.ok())
                {
                    throw std::runtime_error("Failed to finalize Parquet batch: " + status.ToString());
                }
                arrays.emplace_back(std::move(array));
            }

            auto batch = arrow::RecordBatch::Make(layout.schema, static_cast<int64_t>(layout.row_count), std::move(arrays));
            auto status = layout.writer->WriteRecordBatch(*batch);
            if (!status.ok())
            {
                throw std::runtime_error("Failed to write Parquet batch: " + status.ToString());
            }

            layout.row_count = 0;
            rebuild_builders(layout);
        }
        catch (const std::exception &e)
        {
            disable_sink(layout, e.what());
        }
    }

    void ParquetRecorderSink::stop()
    {
        if (disabled)
        {
            return;
        }

        try
        {
            for (auto &layout : layouts)
            {
                flush_batch(layout);
                if (layout.writer != nullptr)
                {
                    auto status = layout.writer->Close();
                    if (!status.ok())
                    {
                        throw std::runtime_error("Failed to close Parquet writer: " + status.ToString());
                    }
                }
            }
        }
        catch (const std::exception &e)
        {
            disable_sink(e.what());
        }
    }
}
