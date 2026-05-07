#include "signal/sinks/parquet_recorder_sink.hpp"

#include "utils/io.hpp"
#include "utils/time.hpp"

#include <arrow/io/api.h>
#include <arrow/result.h>
#include <parquet/properties.h>

#include <algorithm>
#include <chrono>
#include <stdexcept>

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
            auto [column_it, inserted] = column_lookup.emplace(local_name, columns.size());
            if (inserted)
            {
                columns.push_back(ParquetColumnLayout{local_name, variable.type});
            }
            else if (columns[column_it->second].type != variable.type)
            {
                throw std::runtime_error("Parquet column type mismatch for " + local_name);
            }

            ParquetVariableLayout variable_layout;
            variable_layout.name = local_name;
            variable_layout.type = variable.type;
            variable_layout.position = variable.position;
            variable_layout.column = column_it->second;
            layout.variables.emplace_back(std::move(variable_layout));
        }

        layout_lookup[storage] = layout.index;
        layouts.emplace_back(std::move(layout));
    }

    void ParquetRecorderSink::rebuild_builders()
    {
        builders.clear();
        builders.reserve(4 + columns.size());

        builders.emplace_back(make_builder(arrow::int64()));
        builders.emplace_back(make_builder(arrow::float64()));
        builders.emplace_back(make_builder(arrow::utf8()));
        builders.emplace_back(make_builder(arrow::utf8()));

        for (const auto &column : columns)
        {
            builders.emplace_back(make_builder(arrow_type_for(column.type)));
        }
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

            std::vector<std::shared_ptr<arrow::Field>> fields;
            fields.reserve(4 + columns.size());
            fields.emplace_back(make_field("timestamp_ns", arrow::int64()));
            fields.emplace_back(make_field("simulation_time_s", arrow::float64()));
            fields.emplace_back(make_field("model", arrow::utf8()));
            fields.emplace_back(make_field("storage", arrow::utf8()));

            for (const auto &column : columns)
            {
                fields.emplace_back(make_field(column.name, arrow_type_for(column.type)));
            }

            schema = arrow::schema(std::move(fields));
            rebuild_builders();

            auto output_result = arrow::io::FileOutputStream::Open(filename.string());
            if (!output_result.ok())
            {
                throw std::runtime_error("Failed to open Parquet output file: " + output_result.status().ToString());
            }
            output = output_result.ValueOrDie();

            auto writer_props = parquet::WriterProperties::Builder().build();
            auto arrow_props = parquet::ArrowWriterProperties::Builder().store_schema()->build();
            auto writer_result = parquet::arrow::FileWriter::Open(*schema, arrow::default_memory_pool(), output, writer_props, arrow_props);
            if (!writer_result.ok())
            {
                throw std::runtime_error("Failed to create Parquet writer: " + writer_result.status().ToString());
            }
            writer = std::move(writer_result).ValueOrDie();
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
        writer.reset();
        output.reset();
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

        const auto &layout = layouts[layout_it->second];
        const auto simulation_time_s = utils::time::ns_to_s(event.timestamp);
        const auto timestamp_ns = static_cast<std::int64_t>(event.timestamp);

        try
        {
            std::vector<const ParquetVariableLayout *> variables_by_column(columns.size(), nullptr);
            for (const auto &variable : layout.variables)
            {
                variables_by_column[variable.column] = &variable;
            }

            check_status(builder_as<arrow::Int64Builder>(*builders[0])->Append(timestamp_ns), "Failed to append timestamp");
            check_status(builder_as<arrow::DoubleBuilder>(*builders[1])->Append(simulation_time_s), "Failed to append simulation time");
            check_status(builder_as<arrow::StringBuilder>(*builders[2])->Append(layout.model), "Failed to append model");
            check_status(builder_as<arrow::StringBuilder>(*builders[3])->Append(layout.storage_name), "Failed to append storage");

            for (std::size_t i = 0; i < columns.size(); ++i)
            {
                const auto *variable = variables_by_column[i];
                if (variable == nullptr)
                {
                    check_status(builders[4 + i]->AppendNull(), "Failed to append null column");
                    continue;
                }

                append_typed_value(*builders[4 + i], variable->type, event.buffer + variable->position);
            }

            row_count += 1;
            if (row_count >= batch_rows)
            {
                flush_batch();
            }
        }
        catch (const std::exception &e)
        {
            disable_sink(e.what());
        }
    }

    void ParquetRecorderSink::flush_batch()
    {
        if (disabled || writer == nullptr || row_count == 0)
        {
            return;
        }

        try
        {
            std::vector<std::shared_ptr<arrow::Array>> arrays;
            arrays.reserve(builders.size());

            for (auto &builder : builders)
            {
                std::shared_ptr<arrow::Array> array;
                auto status = builder->Finish(&array);
                if (!status.ok())
                {
                    throw std::runtime_error("Failed to finalize Parquet batch: " + status.ToString());
                }
                arrays.emplace_back(std::move(array));
            }

            auto batch = arrow::RecordBatch::Make(schema, static_cast<int64_t>(row_count), std::move(arrays));
            auto status = writer->WriteRecordBatch(*batch);
            if (!status.ok())
            {
                throw std::runtime_error("Failed to write Parquet batch: " + status.ToString());
            }

            row_count = 0;
            rebuild_builders();
        }
        catch (const std::exception &e)
        {
            disable_sink(e.what());
        }
    }

    void ParquetRecorderSink::stop()
    {
        if (disabled)
        {
            return;
        }

        flush_batch();
        if (disabled || writer == nullptr)
        {
            return;
        }

        try
        {
            auto status = writer->Close();
            if (!status.ok())
            {
                throw std::runtime_error("Failed to close Parquet writer: " + status.ToString());
            }
        }
        catch (const std::exception &e)
        {
            disable_sink(e.what());
        }

        writer.reset();
        output.reset();
    }
}
