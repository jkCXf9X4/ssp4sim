#pragma once

#include "ssp4cpp/utils/log.hpp"

#include "config.hpp"

#include <filesystem>
#include <exception>
#include <stdexcept>
#include <string>

namespace ssp4sim
{

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
        std::filesystem::path result_file;

        bool enable_recording;
        bool wait_for_recorder;

        uint64_t recording_interval;

        // Logging

        std::filesystem::path log_file;

        std::string level_terminal;
        std::string level_file;
        std::string level_json;
        std::string level_cutelog;

        SharedConfig(ssp4cpp::utils::log::Logger *log)
        {
            this->log = log;

            LOG_DEBUG(this->log, "[{}] Seting up common config" , __func__);
            // Common
            ssp_path = utils::Config::getString("simulation.ssp");
            ssd = utils::Config::getOr("simulation.ssd", "SystemStructure.ssd");

            working_dir = std::filesystem::path(utils::Config::getOr("simulation.working_dir", "./wd/default"));

            start_time = utils::time::s_to_ns(utils::Config::getDouble("simulation.start_time"));
            end_time = utils::time::s_to_ns(utils::Config::getDouble("simulation.stop_time"));
            timestep = utils::time::s_to_ns(utils::Config::getDouble("simulation.timestep"));
            realtime = utils::Config::getOr("simulation.realtime", false);

            // Recording
            auto default_result_file = working_dir / "result.csv";

            enable_recording = utils::Config::getOr("simulation.recording.enable", true);

            result_file = std::filesystem::path(utils::Config::getOr("simulation.recording.result_file", default_result_file.string()));

            recording_interval = utils::time::s_to_ns(utils::Config::getOr("simulation.recording.interval", 1.0));
            wait_for_recorder = utils::Config::getOr("simulation.recording.wait_for", false);

            // Log
            auto default_log_file = working_dir / "sim.log";

            log_file = std::filesystem::path(utils::Config::getOr("simulation.log.file", default_log_file.string()));

            level_terminal = utils::Config::getOr("simulation.log.level_terminal", "debug");

            level_file = utils::Config::getOr("simulation.log.level_file", "disable");

            level_json = utils::Config::getOr("simulation.log.level_json", "disable");

            level_cutelog = utils::Config::getOr("simulation.log.level_cutelog", "disable");

            LOG_DEBUG(this->log, "Setup of SharedConfig complete");
        }
    };

}
