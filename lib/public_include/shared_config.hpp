#pragma once

#include "ssp4cpp/utils/log.hpp"

#include "config.hpp"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <stdexcept>
#include <string>

namespace ssp4sim
{
    /// Static parameters for the loop-aware (la2) executor family, parsed from
    /// the `simulation.executor.la2.*` key namespace. Consumed directly by
    /// `make_la2_stack` (executor/loop_aware/la2_builder.hpp).
    struct La2Options
    {
        std::string mode = "linear";          // "linear" | "factor" (aliases allowed)
        int iterations = -1;                  // < 0 -> SCC node count
        double factor = 0.8;
        uint64_t threshold = 0.0;               // nanoseconds; free-shrink cutoff for "factor" mode
        bool parallel = false;
    };

    /// Typed executor configuration. Parsed exactly once from the global
    /// `utils::Config` by `ExecutorOptions::load()` (called from the
    /// `SharedConfig` constructor); `ExecutorBuilder` and the executors it
    /// constructs consume these values and never read the global config.
    ///
    /// The legacy `simulation.executor.loop_aware.*` key namespace is
    /// intentionally not read here. `simulation.timestep` is not duplicated
    /// here: the macro step is owned by `FmuModelConfig` (single source of
    /// truth) and handed to the builder alongside these options.
    struct ExecutorOptions
    {
        std::string method = "jacobi";
        bool realtime = false; // outer executor: Realtime vs plain Macro

        // jacobi
        bool jacobi_parallel = false;
        int jacobi_method = 1; // 1 = TBB, 2 = spin pool, 3 = futures
        int workers = 5;

        // seidel
        bool seidel_parallel = false;

        // la2
        La2Options la2;

        /// Read the `simulation.executor.*` subtree (plus `simulation.realtime`
        /// for the outer macro wrapper) from the already-loaded global config.
        /// The single parse site for all executor configuration.
        static ExecutorOptions load();
    };

    inline ExecutorOptions ExecutorOptions::load()
    {
        ExecutorOptions o;
        o.method = utils::Config::getOr("simulation.executor.method", std::string("jacobi"));
        o.realtime = utils::Config::getOr("simulation.realtime", false);

        o.jacobi_parallel = utils::Config::getOr("simulation.executor.jacobi.parallel", false);
        o.jacobi_method = utils::Config::getOr("simulation.executor.jacobi.method", 1);
        o.workers = utils::Config::getOr("simulation.executor.thread_pool_workers", 5);

        o.seidel_parallel = utils::Config::getOr("simulation.executor.seidel.parallel", false);

        o.la2.mode = utils::Config::getOr("simulation.executor.la2.mode", std::string("linear"));
        o.la2.iterations = utils::Config::getOr("simulation.executor.la2.iterations", -1);
        o.la2.factor = utils::Config::getOr("simulation.executor.la2.factor", 0.8);
        o.la2.threshold = utils::time::s_to_ns(utils::Config::getOr("simulation.executor.la2.threshold", 0.0));
        o.la2.parallel = utils::Config::getOr("simulation.executor.la2.parallel", false);
        return o;
    }

    /// Static model-level parameters for `graph::FmuModel`: the experiment
    /// span, tolerance and per-model toggles. Owns the experiment times
    /// (single source of truth for start/stop/timestep); nested in
    /// `SharedConfig` and handed to the model layer as a unit. Parsed
    /// centrally by the `SharedConfig` constructor.
    struct FmuModelConfig
    {
        bool forward_derivatives = true;
        bool fmu_logging = false;
        double tolerance = 0.0;
        uint64_t start_time = 0; // ns
        uint64_t timestep = 0;   // ns
        uint64_t end_time = 0;   // ns
    };

    struct SharedConfig
    {
        ssp4cpp::utils::log::Logger *log;

    public:
        // Common
        std::string ssp_path;
        std::string ssd;

        std::filesystem::path working_dir = "./wd/default";

        // Executor selection and tuning, and model-level parameters (owns the
        // experiment times). Both are the single parse site for their slices.
        ExecutorOptions executor;
        FmuModelConfig fmu;

        // Recordings

        bool enable_recording;
        bool wait_for_recorder;
        bool record_inputs = false;

        struct CsvRecordingConfig
        {
            bool enable = true;
            std::filesystem::path file;
            uint64_t interval = 0;
        };
        CsvRecordingConfig csv;
        struct SqliteRecordingConfig
        {
            bool enable = false;
            std::optional<std::filesystem::path> file;
        };
        SqliteRecordingConfig sqlite;

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

            // Model parameters (owns the experiment times).
            fmu.forward_derivatives = utils::Config::getOr("simulation.executor.forward_derivatives", true);
            fmu.fmu_logging = utils::Config::getOr("simulation.log.fmu", false);
            fmu.tolerance = utils::Config::getDouble("simulation.tolerance");
            fmu.start_time = utils::time::s_to_ns(utils::Config::getDouble("simulation.start_time"));
            fmu.timestep = utils::time::s_to_ns(utils::Config::getDouble("simulation.timestep"));
            fmu.end_time = utils::time::s_to_ns(utils::Config::getDouble("simulation.stop_time"));

            // Executor selection + tuning, parsed centrally here.
            executor = ExecutorOptions::load();

            // Recording

            csv.enable = utils::Config::getOr("simulation.recording.csv.enable", true);
            if (csv.enable)
            {
                const auto csv_interval_s = utils::Config::getOr("simulation.recording.csv.interval", 0.0);
                if (csv_interval_s < 0.0)
                {
                    throw std::runtime_error("simulation.recording.csv.interval must be greater than or equal to zero");
                }
                csv.interval = utils::time::s_to_ns(csv_interval_s);

                auto default_result_file = working_dir / "result.csv";
                csv.file = std::filesystem::path(utils::Config::getOr("simulation.recording.csv.file", default_result_file.string()));
            }

            sqlite.enable = utils::Config::getOr("simulation.recording.sqlite.enable", false);
            if (sqlite.enable)
            {
                // When config key is absent or empty, file is nullopt (auto-generate).
                // When present with non-empty string, use it (backward compat, [TIME] supported).
                const auto file_str = utils::Config::getOr("simulation.recording.sqlite.file", "");
                if (!file_str.empty())
                {
                    sqlite.file = std::filesystem::path( working_dir / file_str);
                }
            }
            wait_for_recorder = utils::Config::getOr("simulation.recording.wait_for", false);
            record_inputs = utils::Config::getOr("simulation.recording.record_inputs", false);

            enable_recording = csv.enable || sqlite.enable;

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