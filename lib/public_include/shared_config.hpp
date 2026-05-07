#pragma once

#include "ssp4cpp/utils/log.hpp"

#include "config.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace ssp4sim
{
    struct SharedConfig
    {
        ssp4cpp::utils::log::Logger *log;

    public:
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

        struct CsvRecordingConfig
        {
            bool enable = true;
            std::filesystem::path file;
            uint64_t interval = 0;
        };
        CsvRecordingConfig csv;
        struct ParquetRecordingConfig
        {
            bool enable = false;
            std::filesystem::path file;
        };
        ParquetRecordingConfig parquet;

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
            if (csv.enable)
            {
                const auto csv_interval_s = utils::Config::getOr("simulation.recording.csv.interval", 1.0);
                if (csv_interval_s < 0.0)
                {
                    throw std::runtime_error("simulation.recording.csv.interval must be greater than or equal to zero");
                }
                csv.interval = utils::time::s_to_ns(csv_interval_s);

                auto default_result_file = working_dir / "result.csv";
                csv.file = std::filesystem::path(utils::Config::getOr("simulation.recording.csv.file", default_result_file.string()));
            }

            parquet.enable = utils::Config::getOr("simulation.recording.parquet.enable", false);
            if (parquet.enable)
            {
                auto default_result_file = working_dir / "result.parquet";
                parquet.file = std::filesystem::path(utils::Config::getOr("simulation.recording.parquet.file", default_result_file.string()));
            }
            wait_for_recorder = utils::Config::getOr("simulation.recording.wait_for", false);

            enable_recording = csv.enable || parquet.enable;

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
