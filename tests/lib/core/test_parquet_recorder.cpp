#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>

#include "signal/sinks/parquet_recorder_sink.hpp"

#include "utils/time.hpp"

#include <arrow/io/api.h>
#include <parquet/arrow/reader.h>

#include <cstdint>
#include <cstddef>
#include <cctype>
#include <cstring>
#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

namespace fs = std::filesystem;
namespace sim_time = ssp4sim::utils::time;

using ssp4sim::signal::NewDataEvent;
using ssp4sim::signal::ParquetRecorderSink;
using ssp4sim::signal::SignalStorage;
using ssp4sim::types::DataType;

namespace
{
    fs::path test_path(const std::string &name)
    {
        return fs::path(SSP4SIM_PROJECT_ROOT) / "build" / name;
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

    fs::path parquet_file(const fs::path &base, std::string_view model, std::string_view storage_name)
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
        filename += ".parquet";
        return base.parent_path() / filename;
    }

    void remove_if_exists(const fs::path &path)
    {
        if (fs::exists(path))
        {
            fs::remove(path);
        }
    }
}

TEST_CASE("Parquet recorder sink writes grouped storage snapshots", "[DataRecorder][Parquet]")
{
    const auto parquet_path = test_path("test_parquet_recorder.parquet");
    const auto consumer_path = parquet_file(parquet_path, "Consumer", "output");
    const auto aux_path = parquet_file(parquet_path, "Aux", "output");
    remove_if_exists(consumer_path);
    remove_if_exists(aux_path);

    ParquetRecorderSink sink(parquet_path);

    SignalStorage storage(1, "Consumer.output");
    storage.add("Consumer.CPUtime", DataType::real, 1);
    storage.add("Consumer.EventCounter", DataType::integer, 1);
    storage.add("Consumer.enabled", DataType::boolean, 1);
    storage.add("Consumer.label", DataType::string, 1);
    storage.allocate();

    SignalStorage aux_storage(2, "Aux.output");
    aux_storage.add("Aux.value", DataType::real, 1);
    aux_storage.allocate();

    sink.on_storage_added(&storage);
    sink.on_storage_added(&aux_storage);
    sink.init();
    sink.start();

    const auto timestamp = 3ULL * sim_time::nanoseconds_per_second + 123ULL;
    const std::size_t area = storage.push(timestamp);
    const std::size_t aux_area = aux_storage.push(timestamp);
    const double cpu_time = 0.045515;
    const int event_counter = 16;
    const int enabled = 1;
    const std::string label = "hello";
    const double aux_value = 9.5;

    std::memcpy(storage.get_item(area, 0), &cpu_time, sizeof(double));
    std::memcpy(storage.get_item(area, 1), &event_counter, sizeof(int));
    std::memcpy(storage.get_item(area, 2), &enabled, sizeof(int));
    auto *label_ptr = reinterpret_cast<std::string *>(storage.get_item(area, 3));
    *label_ptr = label;
    std::memcpy(aux_storage.get_item(aux_area, 0), &aux_value, sizeof(double));

    NewDataEvent event;
    event.storage = &storage;
    event.area = area;
    event.timestamp = timestamp;
    event.buffer = storage.get_item(area, 0);
    event.recorder_storage_index = 0;

    NewDataEvent aux_event;
    aux_event.storage = &aux_storage;
    aux_event.area = aux_area;
    aux_event.timestamp = timestamp;
    aux_event.buffer = aux_storage.get_item(aux_area, 0);
    aux_event.recorder_storage_index = 1;

    REQUIRE_NOTHROW(sink.on_event(event));
    REQUIRE_NOTHROW(sink.on_event(aux_event));
    REQUIRE_NOTHROW(sink.stop());

    REQUIRE(fs::exists(consumer_path));
    REQUIRE(fs::exists(aux_path));

    auto input_result = arrow::io::ReadableFile::Open(consumer_path.string());
    REQUIRE(input_result.ok());
    auto input = input_result.ValueOrDie();

    auto reader_result = parquet::arrow::OpenFile(input, arrow::default_memory_pool());
    REQUIRE(reader_result.ok());
    auto reader = std::move(reader_result).ValueOrDie();

    std::shared_ptr<arrow::Table> table;
    REQUIRE(reader->ReadTable(&table).ok());
    REQUIRE(table->num_rows() == 1);

    const auto timestamp_column = table->GetColumnByName("timestamp_ns");
    const auto sim_time_column = table->GetColumnByName("simulation_time_s");
    const auto model_column = table->GetColumnByName("model");
    const auto storage_column = table->GetColumnByName("storage");
    const auto cpu_column = table->GetColumnByName("CPUtime");
    const auto counter_column = table->GetColumnByName("EventCounter");
    const auto enabled_column = table->GetColumnByName("enabled");
    const auto label_column = table->GetColumnByName("label");

    REQUIRE(timestamp_column != nullptr);
    REQUIRE(sim_time_column != nullptr);
    REQUIRE(model_column != nullptr);
    REQUIRE(storage_column != nullptr);
    REQUIRE(cpu_column != nullptr);
    REQUIRE(counter_column != nullptr);
    REQUIRE(enabled_column != nullptr);
    REQUIRE(label_column != nullptr);

    const auto timestamp_ns = std::static_pointer_cast<arrow::Int64Array>(timestamp_column->chunk(0));
    const auto sim_time_s = std::static_pointer_cast<arrow::DoubleArray>(sim_time_column->chunk(0));
    const auto model = std::static_pointer_cast<arrow::StringArray>(model_column->chunk(0));
    const auto storage_name = std::static_pointer_cast<arrow::StringArray>(storage_column->chunk(0));
    const auto cpu = std::static_pointer_cast<arrow::DoubleArray>(cpu_column->chunk(0));
    const auto counter = std::static_pointer_cast<arrow::Int64Array>(counter_column->chunk(0));
    const auto enabled_value = std::static_pointer_cast<arrow::BooleanArray>(enabled_column->chunk(0));
    const auto label_value = std::static_pointer_cast<arrow::StringArray>(label_column->chunk(0));

    REQUIRE(timestamp_ns->Value(0) == static_cast<std::int64_t>(timestamp));
    REQUIRE(sim_time_s->Value(0) == Catch::Approx(sim_time::ns_to_s(timestamp)));
    REQUIRE(model->GetString(0) == "Consumer");
    REQUIRE(storage_name->GetString(0) == "output");
    REQUIRE(cpu->Value(0) == Catch::Approx(cpu_time));
    REQUIRE(counter->Value(0) == event_counter);
    REQUIRE(enabled_value->Value(0) == true);
    REQUIRE(label_value->GetString(0) == "hello");

    auto aux_input_result = arrow::io::ReadableFile::Open(aux_path.string());
    REQUIRE(aux_input_result.ok());
    auto aux_input = aux_input_result.ValueOrDie();

    auto aux_reader_result = parquet::arrow::OpenFile(aux_input, arrow::default_memory_pool());
    REQUIRE(aux_reader_result.ok());
    auto aux_reader = std::move(aux_reader_result).ValueOrDie();

    REQUIRE(aux_reader->ReadTable(&table).ok());
    REQUIRE(table->num_rows() == 1);

    const auto aux_model_column = table->GetColumnByName("model");
    const auto aux_storage_column = table->GetColumnByName("storage");
    const auto aux_value_column = table->GetColumnByName("value");

    REQUIRE(aux_model_column != nullptr);
    REQUIRE(aux_storage_column != nullptr);
    REQUIRE(aux_value_column != nullptr);

    const auto aux_model_value = std::static_pointer_cast<arrow::StringArray>(aux_model_column->chunk(0));
    const auto aux_storage_value = std::static_pointer_cast<arrow::StringArray>(aux_storage_column->chunk(0));
    const auto aux_value_array = std::static_pointer_cast<arrow::DoubleArray>(aux_value_column->chunk(0));

    REQUIRE(aux_model_value->GetString(0) == "Aux");
    REQUIRE(aux_storage_value->GetString(0) == "output");
    REQUIRE(aux_value_array->Value(0) == Catch::Approx(aux_value));

    remove_if_exists(consumer_path);
    remove_if_exists(aux_path);
}
