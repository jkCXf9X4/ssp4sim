#include "simulator.hpp"

#include "config.hpp"
#include "simulation.hpp"
#include "ssp4cpp/ssp.hpp"
#include "ssp4cpp/utils/log.hpp"

#include <filesystem>
#include <exception>
#include <stdexcept>

namespace ssp4sim
{
    struct SimulatorPrivate
    {
        ssp4cpp::utils::log::Logger* log;

        std::unique_ptr<ssp4cpp::Ssp> ssp;
        std::unique_ptr<Simulation> sim;
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
        // p->log->flush_log(); 

        auto log_file = utils::Config::getString("simulation.log.file");

        ssp4cpp::utils::log::init_logging();
        ssp4cpp::utils::log::add_console(quill::loglevel_from_string(utils::Config::getOr("simulation.log.level_terminal", "info")));
        ssp4cpp::utils::log::add_file_sink(log_file, quill::loglevel_from_string(utils::Config::getOr("simulation.log.level_file", "info")));
        ssp4cpp::utils::log::add_json_sink(log_file + ".json",  quill::loglevel_from_string(utils::Config::getOr("simulation.log.level_json", "tracel1")));
        // Do not construct any objects using a logger before this point, they wont get any sinks.
        
        p->log = ssp4cpp::utils::log::make_logger("ssp4sim.Simulator");

        LOG_DEBUG(p->log, "[{func}] - Importing SSP", __func__);
        auto ssp_path = utils::Config::getString("simulation.ssp");
        auto ssd = utils::Config::getOr("simulation.ssd", std::string("SystemStructure.ssd"));
        p->ssp = std::make_unique<ssp4cpp::Ssp>(ssp_path, ssd);
        LOG_DEBUG(p->log, "[{func}] -- SSP: {ssp}", __func__, p->ssp->to_string());

        LOG_DEBUG(p->log, "[{func}] - Creating simulation\n", __func__);
        p->sim = std::make_unique<Simulation>(p->ssp.get());
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
