#pragma once

#include "ssp4cpp/utils/log.hpp"

#include "config.hpp"

#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <exception>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>

namespace ssp4sim
{
    struct InfluxRecordingConfig
    {
        bool enable = false;
        std::string url;
        std::string db;
        std::string token;
        std::string measurement = "ssp4sim_signal";
        std::string run = "run_[TIME]";
        std::size_t batch_size = 500;
    };

    inline std::string discover_influx_token()
    {
        if (const char *env_token = std::getenv("SSP4SIM_INFLUX_TOKEN"); env_token != nullptr && *env_token != '\0')
        {
            return env_token;
        }

        const char *home = std::getenv("HOME");
        if (home == nullptr || *home == '\0')
        {
            return {};
        }

        const auto config_path = std::filesystem::path(home) / ".influxdb" / "docker" / "explorer" / "config" / "config.json";
        std::ifstream stream(config_path);
        if (!stream)
        {
            return {};
        }

        const auto config = nlohmann::json::parse(stream, nullptr, false);
        if (config.is_discarded() || !config.is_object())
        {
            return {};
        }

        return config.value("DEFAULT_API_TOKEN", "");
    }

    struct SharedConfig
    {
        ssp4cpp::utils::log::Logger *log;

        // Common
        std::string ssp_path;
        std::string ssd;

        std::filesystem::path working_dir;

        uint64_t start_time;
        uint64_t end_time;
        uint64_t timestep;
        bool realtime;

        // Recordings

        bool enable_recording;
        bool wait_for_recorder;

        uint64_t recording_interval;

        struct CsvRecordingConfig
        {
            bool enable = true;
            std::filesystem::path file;
        };
        CsvRecordingConfig csv;

        InfluxRecordingConfig influx;

        // Logging

        std::filesystem::path log_file;
        std::filesystem::path start_value_log_file;

        std::string level_terminal;
        std::string level_file;
        std::string level_json;
        std::string level_cutelog;

        SharedConfig(ssp4cpp::utils::log::Logger *log)
        {
            this->log = log;

            LOG_DEBUG(this->log, "[{}] Seting up common config", __func__);
            // Common
            ssp_path = utils::Config::getString("simulation.ssp");
            ssd = utils::Config::getOr("simulation.ssd", "SystemStructure.ssd");

            working_dir = std::filesystem::path(utils::Config::getOr("simulation.working_dir", "./wd/default"));

            start_time = utils::time::s_to_ns(utils::Config::getDouble("simulation.start_time"));
            end_time = utils::time::s_to_ns(utils::Config::getDouble("simulation.stop_time"));
            timestep = utils::time::s_to_ns(utils::Config::getDouble("simulation.timestep"));
            realtime = utils::Config::getOr("simulation.realtime", false);

            // Recording

            csv.enable = utils::Config::getOr("simulation.recording.csv.enable", false);

            auto default_result_file = working_dir / "result.csv";
            csv.file = std::filesystem::path(utils::Config::getOr("simulation.recording.csv.file", default_result_file.string()));

            influx.enable = utils::Config::getOr("simulation.recording.influx.enable", false);
            if (influx.enable)
            {
                LOG_INFO(this->log, "[{}] Influx recording enabled", __func__);

                influx.url = utils::Config::getOr("simulation.recording.influx.url", "http://localhost:8181");
                LOG_INFO(this->log, "[{}] Influx url: {}", __func__, influx.url);

                influx.db = utils::Config::getOr("simulation.recording.influx.db", "ssp4sim");
                LOG_INFO(this->log, "[{}] Influx db: {}", __func__, influx.db);

                influx.token = utils::Config::getOr("simulation.recording.influx.token", std::string{});
                if (influx.token.empty())
                {
                    influx.token = discover_influx_token();
                    if (influx.token.empty())
                    {
                        throw std::runtime_error("Influx recording is enabled but no access token could be identified.");
                    }
                }

                influx.measurement = utils::Config::getOr("simulation.recording.influx.measurement", "ssp4sim_signal");

                influx.run = utils::Config::getOr("simulation.recording.influx.run", "run_[TIME]");

                auto batch_size = utils::Config::getOr("simulation.recording.influx.batch_size", 500);
                if (batch_size <= 0)
                {
                    throw std::runtime_error("simulation.recording.influx.batch_size must be greater than zero");
                }
                influx.batch_size = static_cast<std::size_t>(batch_size);
            }

            recording_interval = utils::time::s_to_ns(utils::Config::getOr("simulation.recording.interval", 1.0));
            wait_for_recorder = utils::Config::getOr("simulation.recording.wait_for", false);

            enable_recording = csv.enable || influx.enable;

            // Log
            auto default_log_file = working_dir / "sim.log";

            log_file = std::filesystem::path(utils::Config::getOr("simulation.log.file", default_log_file.string()));

            level_terminal = utils::Config::getOr("simulation.log.level_terminal", "debug");

            level_file = utils::Config::getOr("simulation.log.level_file", "disable");

            level_json = utils::Config::getOr("simulation.log.level_json", "disable");

            level_cutelog = utils::Config::getOr("simulation.log.level_cutelog", "disable");

            auto default_start_value_log_file = utils::Config::getOr("simulation.log.start_values", "start_values.csv");

            start_value_log_file = working_dir / default_start_value_log_file;

            LOG_DEBUG(this->log, "Setup of SharedConfig complete");
        }
    };

}
