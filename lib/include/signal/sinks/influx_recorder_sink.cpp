#include "signal/sinks/influx_recorder_sink.hpp"

#include "signal/sinks/writers/influx_http_writer.hpp"
#include "signal/sinks/writers/influx_udp_writer.hpp"

#include "utils/time.hpp"

#include <chrono>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

namespace ssp4sim::signal
{
    namespace
    {
        struct StorageNameParts
        {
            std::string_view model;
            std::string_view storage;
            bool has_model = false;
        };

        StorageNameParts split_storage_name(std::string_view storage_name)
        {
            const auto dot = storage_name.find('.');
            if (dot == std::string_view::npos)
            {
                return {{}, storage_name, false};
            }

            return {storage_name.substr(0, dot), storage_name.substr(dot + 1), true};
        }

        std::string make_field_key(std::string_view storage_name, std::string_view variable_name)
        {
            const auto parts = split_storage_name(storage_name);
            const auto prefix = parts.has_model ? parts.model : storage_name;

            if (variable_name.size() > prefix.size() + 1
                && variable_name.compare(0, prefix.size(), prefix) == 0
                && variable_name[prefix.size()] == '.')
            {
                return escape_tag(variable_name.substr(prefix.size() + 1));
            }

            return escape_tag(variable_name);
        }
    }

    InfluxRecorderSink::InfluxRecorderSink(
        ssp4sim::InfluxRecordingConfig influx,
        std::chrono::system_clock::time_point run_start_wall_clock,
        std::unique_ptr<InfluxWriter> writer)
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.signal.InfluxRecorderSink")),
          config(influx),
          run_start_wall_clock(run_start_wall_clock),
          writer(std::move(writer))
    {

        LOG_DEBUG(log, "[{func}] Measurement: {measurement}, run: {run}", __func__, config.measurement, config.run);
        LOG_DEBUG(log, "[{func}] Protocol: {protocol}", __func__, config.protocol);
        LOG_DEBUG(log, "[{func}] Batch size: {batch_size}", __func__, config.batch_size);
        LOG_DEBUG(log, "[{func}] Auth token: {token}", __func__, config.token.empty() ? "disabled" : "enabled");
    }

    void InfluxRecorderSink::on_storage_added(const SignalStorage *storage)
    {
        if (storage == nullptr || storage->mem_size == 0)
        {
            return;
        }

        if (layout_lookup.contains(storage))
        {
            LOG_WARNING(log, "[{func}] Ignoring duplicate storage: {}", __func__, storage->name);
            return;
        }

        InfluxStorageLayout layout;
        layout.storage = storage;
        layout.index = layouts.size();
        const auto parts = split_storage_name(storage->name);
        layout.line_prefix = escape_measurement(config.measurement);
        layout.line_prefix += ",run=" + escape_tag(config.run);
        if (parts.has_model)
        {
            layout.has_model_tag = true;
            layout.model_tag = escape_tag(parts.model);
            layout.storage_tag = escape_tag(parts.storage);
            layout.line_prefix += ",model=" + layout.model_tag;
            layout.line_prefix += ",storage=" + layout.storage_tag;
        }
        else
        {
            layout.storage_tag = escape_tag(parts.storage);
            layout.line_prefix += ",storage=" + layout.storage_tag;
        }
        layout.variables.reserve(storage->variables.size());

        for (const auto &variable : storage->variables)
        {
            InfluxVariableLayout variable_layout;
            variable_layout.name = variable.name;
            variable_layout.type = variable.type;
            variable_layout.position = variable.position;
            variable_layout.field_key = make_field_key(storage->name, variable.name);
            layout.variables.emplace_back(std::move(variable_layout));
        }

        layout_lookup[storage] = layout.index;
        layouts.emplace_back(std::move(layout));
    }

    void InfluxRecorderSink::init()
    {
        LOG_TRACE_L1(log, "[{func}] Init", __func__);

        if (initialized)
        {
            return;
        }

        if (config.batch_size == 0)
        {
            disable_sink("batch size must be greater than zero");
            return;
        }

        try
        {
            if (!writer)
            {
                if (config.protocol == "udp")
                {
                    writer = std::make_unique<UdpInfluxWriter>(config.host, config.port);
                }
                else
                {
                    auto url = "http://" + config.host + ":" + config.port + "/api/v3/write_lp?db=" + config.db;
                    writer = std::make_unique<InfluxHttpWriter>(url, config.token);
                }
            }

            writer->batch_of(config.batch_size);
            initialized = true;
            disabled = false;
        }
        catch (const std::exception &e)
        {
            disable_sink(e.what());
        }
    }

    void InfluxRecorderSink::start()
    {
        started = true;
        ensure_run_start_initialized();
    }

    void InfluxRecorderSink::ensure_run_start_initialized()
    {
        if (run_start_wall_clock == std::chrono::system_clock::time_point{})
        {
            run_start_wall_clock = utils::time::time_now();
        }
    }

    bool InfluxRecorderSink::should_record(InfluxStorageLayout &layout, std::uint64_t timestamp) const
    {
        if (config.interval == 0)
        {
            return true;
        }

        if (!layout.has_recorded_timestamp || timestamp >= layout.last_recorded_timestamp + config.interval)
        {
            layout.last_recorded_timestamp = timestamp;
            layout.has_recorded_timestamp = true;
            return true;
        }

        return false;
    }

    std::string InfluxRecorderSink::build_line_protocol(
        const InfluxStorageLayout &layout,
        const std::byte *data,
        double simulation_time_s,
        std::int64_t timestamp_ns) const
    {
        std::string line = layout.line_prefix;
        line.reserve(line.size() + layout.variables.size() * 32 + 32);
        line += ' ';

        bool first_field = true;
        auto append_field_separator = [&]()
        {
            if (!first_field)
            {
                line += ',';
            }
            first_field = false;
        };

        // The recorder has already copied the source area into aligned,
        // recorder-owned storage, so these values can be read directly.
        for (const auto &variable : layout.variables)
        {
            append_field_separator();
            line += variable.field_key;
            line += '=';

            const auto *value = data + variable.position;
            switch (variable.type)
            {
            case types::DataType::real:
            {
                append_double(line, *reinterpret_cast<const double *>(value));
                break;
            }
            case types::DataType::boolean:
            {
                append_integral(line, *reinterpret_cast<const int *>(value) ? 1 : 0);
                break;
            }
            case types::DataType::integer:
            case types::DataType::enumeration:
            {
                append_integral(line, *reinterpret_cast<const int *>(value));
                break;
            }
            case types::DataType::string:
            {
                const auto *string_value = reinterpret_cast<const std::string *>(value);
                line += escape_string_field(*string_value);
                break;
            }
            default:
                throw std::runtime_error("Unsupported variable type for signal " + variable.name);
            }
        }

        append_field_separator();
        line += "simulation_time_s=";
        append_double(line, simulation_time_s);
        line += ' ';
        append_integral(line, timestamp_ns);
        return line;
    }

    void InfluxRecorderSink::disable_sink(const std::string &reason)
    {
        if (disabled)
        {
            return;
        }

        disabled = true;
        LOG_WARNING(log, "[{func}] Influx sink disabled: {}", __func__, reason);
    }

    void InfluxRecorderSink::on_event(const NewDataEvent &event)
    {
        if (disabled || !initialized || !writer || event.storage == nullptr || event.buffer == nullptr)
        {
            return;
        }

        auto layout_it = layout_lookup.find(event.storage);
        if (layout_it == layout_lookup.end())
        {
            LOG_WARNING(log, "[{func}] Ignoring event for unknown storage {}", __func__, event.storage->name);
            return;
        }

        ensure_run_start_initialized();

        auto &layout = layouts[layout_it->second];
        if (!should_record(layout, event.timestamp))
        {
            return;
        }

        const auto simulation_time_s = utils::time::ns_to_s(event.timestamp);
        const auto timestamp = run_start_wall_clock + std::chrono::nanoseconds(event.timestamp);
        const auto timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(timestamp.time_since_epoch()).count();

        try
        {
            writer->write(build_line_protocol(layout, event.buffer, simulation_time_s, timestamp_ns));
        }
        catch (const std::exception &e)
        {
            disable_sink(e.what());
        }
    }

    void InfluxRecorderSink::stop()
    {
        if (!writer)
        {
            return;
        }

        try
        {
            writer->flush_batch();
        }
        catch (const std::exception &e)
        {
            disable_sink(e.what());
        }
    }
}
