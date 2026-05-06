#include "signal/influx_recorder_sink.hpp"

#include <cpr/cpr.h>

#include "utils/time.hpp"

#include <cstring>
#include <chrono>
#include <stdexcept>
#include <string>
#include <vector>
#include <utility>

namespace ssp4sim::signal
{
    namespace
    {
        std::string to_line_protocol(const influxdb::Point &point)
        {
            std::string line = point.getName();

            const auto tags = point.getTags();
            if (!tags.empty())
            {
                line += ",";
                line += tags;
            }

            line += " ";
            line += point.getFields();

            const auto timestamp_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(point.getTimestamp().time_since_epoch()).count();
            line += " ";
            line += std::to_string(timestamp_ns);

            return line;
        }

        std::string with_nanosecond_precision(const std::string &url)
        {
            if (url.find("precision=") != std::string::npos)
            {
                return url;
            }

            if (url.find('?') == std::string::npos)
            {
                return url + "?precision=ns";
            }

            return url + "&precision=ns";
        }

        class InfluxHttpWriter final : public InfluxWriter
        {
        public:
            explicit InfluxHttpWriter(std::string url, std::string token)
                : url(with_nanosecond_precision(url)),
                  token(std::move(token))
            {
            }

            void batch_of(std::size_t size) override
            {
                batch_size = size == 0 ? 1 : size;
                pending.reserve(batch_size);
            }

            void write(influxdb::Point point) override
            {
                auto line = to_line_protocol(point);
                pending_bytes += line.size() + 1;
                pending.emplace_back(std::move(line));
                if (pending.size() >= batch_size)
                {
                    flush_pending();
                }
            }

            void flush_batch() override
            {
                flush_pending();
            }

        private:
            std::string url;
            std::string token;
            std::size_t batch_size = 1;
            std::vector<std::string> pending;
            std::size_t pending_bytes = 0;

            void flush_pending()
            {
                if (pending.empty())
                {
                    return;
                }

                std::string body;
                body.reserve(pending_bytes == 0 ? 0 : pending_bytes - 1);
                for (std::size_t i = 0; i < pending.size(); ++i)
                {
                    if (i > 0)
                    {
                        body += '\n';
                    }
                    body += pending[i];
                }

                cpr::Header headers{{"Content-Type", "text/plain; charset=utf-8"}};
                if (!token.empty())
                {
                    headers.emplace("Authorization", "Bearer " + token);
                }

                const auto response = cpr::Post(
                    cpr::Url{url},
                    headers,
                    cpr::Body{body},
                    cpr::Timeout{5000},
                    cpr::ConnectTimeout{1000});

                if (response.error.code != cpr::ErrorCode::OK)
                {
                    throw std::runtime_error("Influx write failed: " + response.error.message);
                }

                if (response.status_code < 200 || response.status_code >= 300)
                {
                    throw std::runtime_error("Influx write failed with HTTP status " + std::to_string(response.status_code) + ": " + response.text);
                }

                pending.clear();
                pending_bytes = 0;
            }
        };
    }

    InfluxRecorderSink::InfluxRecorderSink(
        ssp4sim::InfluxRecordingConfig influx,
        std::chrono::system_clock::time_point run_start_wall_clock,
        std::unique_ptr<InfluxWriter> writer)
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.signal.InfluxRecorderSink")),
          token(influx.token),
          measurement(influx.measurement),
          run_name(influx.run),
          batch_size(influx.batch_size),
          run_start_wall_clock(run_start_wall_clock),
          writer(std::move(writer))
    {
        url = influx.url + "/api/v3/write_lp?db=" + influx.db;

        LOG_DEBUG(log, "[{func}] URL: {url}, measurement: {measurement}, run: {run}", __func__, this->url, this->measurement, this->run_name);
        LOG_DEBUG(log, "[{func}] Batch size: {batch_size}", __func__, this->batch_size);
        LOG_DEBUG(log, "[{func}] Auth token: {token}", __func__, this->token.empty() ? "disabled" : "enabled");
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

        if (batch_size == 0)
        {
            disable_sink("batch size must be greater than zero");
            return;
        }

        try
        {
            if (!writer)
            {
                writer = std::make_unique<InfluxHttpWriter>(url, token);
            }

            writer->batch_of(batch_size);
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

    std::optional<influxdb::Point::FieldValue> InfluxRecorderSink::read_field_value(const std::byte *data, types::DataType type) const
    {
        switch (type)
        {
        case types::DataType::real:
        {
            double value = 0.0;
            std::memcpy(&value, data, sizeof(value));
            return value;
        }
        case types::DataType::boolean:
        {
            bool value = false;
            std::memcpy(&value, data, sizeof(value));
            return value ? 1.0 : 0.0;
        }
        case types::DataType::integer:
        case types::DataType::enumeration:
        {
            int value = 0;
            std::memcpy(&value, data, sizeof(value));
            return static_cast<double>(value);
        }
        case types::DataType::string:
        {
            const auto *value = reinterpret_cast<const std::string *>(data);
            return *value;
        }
        default:
            return std::nullopt;
        }
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

        const auto &layout = layouts[layout_it->second];
        const auto simulation_time_s = utils::time::ns_to_s(event.timestamp);
        const auto timestamp = run_start_wall_clock + std::chrono::nanoseconds(event.timestamp);

        for (const auto &variable : layout.variables)
        {
            const auto *data = event.buffer + variable.position;
            auto field_value = read_field_value(data, variable.type);
            if (!field_value.has_value())
            {
                LOG_WARNING(log, "[{func}] Skipping unsupported variable type {} for signal {}", __func__, variable.type.to_string(), variable.name);
                continue;
            }

            try
            {
                influxdb::Point point(measurement);
                const auto value_field = variable.type == types::DataType::string ? "value_string" : "value";
                point.addTag("run", run_name)
                    .addTag("storage", event.storage->name)
                    .addTag("signal", variable.name)
                    .addTag("type", variable.type.to_string())
                    .addField(value_field, *field_value)
                    .addField("simulation_time_s", simulation_time_s)
                    .setTimestamp(timestamp);

                writer->write(std::move(point));
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
