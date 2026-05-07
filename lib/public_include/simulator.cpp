#include "simulator.hpp"

#include "config.hpp"
#include "simulation.hpp"
#include "ssp4cpp/ssp.hpp"
#include "ssp4cpp/utils/log.hpp"
#include "shared_config.hpp"

#include <filesystem>
#include <exception>
#include <stdexcept>
#include <string>

namespace ssp4sim
{
    struct SimulatorPrivate
    {
        ssp4cpp::utils::log::Logger* log;

        std::unique_ptr<ssp4cpp::Ssp> ssp;
        std::unique_ptr<Simulation> sim;
        std::unique_ptr<SharedConfig> conf;
    };


    Simulator::Simulator(const std::string config_path)
        : p(std::make_unique<SimulatorPrivate>())
    {
        p->log = ssp4cpp::utils::log::simple_logger();
        LOG_INFO(p->log, "[{func}] Setting up Simulator", __func__);

        LOG_INFO(p->log, "[{func}] - Loading config: {config}", __func__, config_path);
        if (!std::filesystem::exists(config_path))
        {
            LOG_ERROR(p->log, "Config file does not exist: {config_path}", config_path);
            throw std::runtime_error("Config file does not exist: " + config_path);
        }

        utils::Config::loadFromFile(config_path);
        LOG_DEBUG(p->log, "[{func}] - Config loaded:\n{config}\n", __func__, utils::Config::as_string());

        // need to be constructed after Config::loadFromFile
        p->conf = std::make_unique<SharedConfig>(p->log);

        LOG_INFO(p->log, "[{func}] - Working directory: {}", __func__, p->conf->working_dir.c_str());
        
        std::filesystem::create_directories(p->conf->working_dir);

        if (!p->conf->log_file.parent_path().empty())
        {
            std::filesystem::create_directories(p->conf->log_file.parent_path());
        }

        if (p->conf->level_terminal != "disable")
        {
            ssp4cpp::utils::log::add_console(quill::loglevel_from_string(p->conf->level_terminal));
        }
        
        if (p->conf->level_file != "disable")
        {
            ssp4cpp::utils::log::add_file_sink(p->conf->log_file.string(), quill::loglevel_from_string(p->conf->level_file));
        }
        
        if (p->conf->level_json != "disable")
        {
            ssp4cpp::utils::log::add_json_sink(p->conf->log_file.string() + ".json",  quill::loglevel_from_string(p->conf->level_json));
        }
        
        if (p->conf->level_cutelog != "disable")
        {
            try
            {
                ssp4cpp::utils::log::add_cutelog_sink("127.0.0.1", 19996, quill::loglevel_from_string(p->conf->level_cutelog));
            }
            catch (const std::exception &e)
            {
                LOG_WARNING(p->log, "[{func}] Cutelog sink disabled: {}", __func__, e.what());
            }
        }

        ssp4cpp::utils::log::init_logging();

        p->log = ssp4cpp::utils::log::make_logger("ssp4sim.Simulator");

        LOG_DEBUG(p->log, "[{func}] - Importing SSP", __func__);
        p->ssp = std::make_unique<ssp4cpp::Ssp>(p->conf->ssp_path, p->conf->ssd);
        LOG_DEBUG(p->log, "[{func}] -- SSP: {ssp}", __func__, p->ssp->to_string());

        LOG_DEBUG(p->log, "[{func}] - Creating simulation\n", __func__);
        p->sim = std::make_unique<Simulation>(p->ssp.get(), p->conf.get());
    }

    Simulator::~Simulator() = default;

    void Simulator::init()
    {
        if (!p || !p->sim)
        {
            throw std::runtime_error("Simulator is not initialized");
        }
        LOG_INFO(p->log, "[{func}] Initializing Simulator\n", __func__);
        p->sim->init();
    }

    void Simulator::simulate()
    {
        if (!p || !p->sim)
        {
            throw std::runtime_error("Simulator is not initialized");
        }
        LOG_INFO(p->log, "[{func}] Starting Simulator\n", __func__);
        p->sim->simulate();
    }

}
