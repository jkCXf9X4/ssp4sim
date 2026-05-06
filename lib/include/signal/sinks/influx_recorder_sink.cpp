#include "signal/sinks/influx_recorder_sink.hpp"

#include "signal/sinks/writers/influx_http_writer.hpp"
#include "signal/sinks/writers/influx_udp_writer.hpp"

#include "utils/ip.hpp"
#include "utils/time.hpp"

#include <chrono>
#include <stdexcept>
#include <string>
#include <utility>

namespace ssp4sim::signal
{
    InfluxRecorderSink::InfluxRecorderSink(
        ssp4sim::InfluxRecordingConfig influx,
        std::chrono::system_clock::time_point run_start_wall_clock,
        std::unique_ptr<InfluxWriter> writer)
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.signal.InfluxRecorderSink")),
          config(influx),
          run_start_wall_clock(run_start_wall_clock),
          writer(std::move(writer))
    {

        LOG_DEBUG(log, "[{func}] URL: {url}, run: {run}", __func__, config.measurement, config.run);
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
        layout.variables.reserve(storage->variables.size());

        for (const auto &variable : storage->variables)
        {
            InfluxVariableLayout variable_layout;
            variable_layout.name = variable.name;
            variable_layout.type = variable.type;
            variable_layout.position = variable.position;
            variable_layout.string_value = variable.type == types::DataType::string;
            variable_layout.line_prefix = escape_measurement(config.measurement);
            variable_layout.line_prefix += ",run=" + escape_tag(config.run);
            variable_layout.line_prefix += ",storage=" + escape_tag(storage->name);
            variable_layout.line_prefix += ",signal=" + escape_tag(variable.name);
            variable_layout.line_prefix += ",type=" + escape_tag(variable.type.to_string());
            variable_layout.line_prefix += (variable_layout.string_value ? " value_string=" : " value=");
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
        const InfluxVariableLayout &variable,
        const std::byte *data,
        double simulation_time_s,
        std::int64_t timestamp_ns) const
    {
        std::string line = variable.line_prefix;

        // The recorder has already copied the source area into aligned,
        // recorder-owned storage, so these values can be read directly.
        switch (variable.type)
        {
        case types::DataType::real:
        {
            append_double(line, *reinterpret_cast<const double *>(data));
            break;
        }
        case types::DataType::boolean:
        {
            append_integral(line, *reinterpret_cast<const int *>(data) ? 1 : 0);
            break;
        }
        case types::DataType::integer:
        case types::DataType::enumeration:
        {
            append_integral(line, *reinterpret_cast<const int *>(data));
            break;
        }
        case types::DataType::string:
        {
            const auto *value = reinterpret_cast<const std::string *>(data);
            line += escape_string_field(*value);
            break;
        }
        default:
            throw std::runtime_error("Unsupported variable type for signal " + variable.name);
        }

        line += ",simulation_time_s=";
        append_double(line, simulation_time_s);
        line += " ";
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

        for (const auto &variable : layout.variables)
        {
            const auto *data = event.buffer + variable.position;

            try
            {
                writer->write(build_line_protocol(variable, data, simulation_time_s, timestamp_ns));
            }
            catch (const std::exception &e)
            {
                disable_sink(e.what());
                return;
            }
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
