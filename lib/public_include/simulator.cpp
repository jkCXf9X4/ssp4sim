#include "simulator.hpp"

#include "config.hpp"
#include "simulation.hpp"
#include "ssp4cpp/ssp.hpp"
#include "ssp4cpp/utils/log.hpp"
#include "quill/SimpleSetup.h"

#include "cutecpp/log.hpp"

#include <filesystem>
#include <exception>
#include <stdexcept>

namespace ssp4sim
{
    struct SimulatorPrivate
    {
        quill::Logger* log;

        std::unique_ptr<ssp4cpp::Ssp> ssp;
        std::unique_ptr<Simulation> sim;
    };


    Simulator::Simulator(const std::string config_path)
        : p(std::make_unique<SimulatorPrivate>())
    {
        ssp4cpp::utils::log::init_logging();
        p->log = quill::simple_logger();
        LOG_INFO(p->log, "[{}] Setting up Simulator", __func__);

        LOG_INFO(p->log, "[{}] - Loading config: {}", __func__, config_path);
        if (!std::filesystem::exists(config_path))
        {
            LOGV_ERROR(p->log, "Config file does not exist", config_path);
            throw std::runtime_error("Config file does not exist: " + config_path);
        }
        
        utils::Config::loadFromFile(config_path);
        LOG_DEBUG(p->log, "[{}] - Config loaded:\n{}\n", __func__, utils::Config::as_string());
        // p->log->flush_log(); 

        auto log_file = utils::Config::getString("simulation.log.file");

        ssp4cpp::utils::log::add_console(quill::loglevel_from_string(utils::Config::getString("simulation.log.level_terminal")));
        ssp4cpp::utils::log::add_file_sink(log_file, quill::loglevel_from_string(utils::Config::getString("simulation.log.level_file")));
        ssp4cpp::utils::log::add_json_sink(log_file + ".json",  quill::loglevel_from_string(utils::Config::getString("simulation.log.level_json")));
        // Do not construct any objects using a logger before this point, they wont get any sinks.
        
        p->log = ssp4cpp::utils::log::make_logger("ssp4sim.Simulator", quill::LogLevel::TraceL1);

        LOG_DEBUG(p->log, "[{}] - Importing SSP", __func__);
        auto ssp_path = utils::Config::getString("simulation.ssp");
        auto ssd = utils::Config::getOr("simulation.ssd", std::string("SystemStructure.ssd"));
        p->ssp = std::make_unique<ssp4cpp::Ssp>(ssp_path, ssd);
        LOG_DEBUG(p->log, "[{}] -- SSP: {}", __func__, p->ssp->to_string());

        LOG_DEBUG(p->log, "[{}] - Creating simulation\n", __func__);
        p->sim = std::make_unique<Simulation>(p->ssp.get());
    }

    Simulator::~Simulator() = default;

    void Simulator::init()
    {
        if (!p || !p->sim)
        {
            throw std::runtime_error("Simulator is not initialized");
        }
        LOG_INFO(p->log, "[{}] Initializing Simulator\n", __func__);
        p->sim->init();
    }

    void Simulator::simulate()
    {
        if (!p || !p->sim)
        {
            throw std::runtime_error("Simulator is not initialized");
        }
        LOG_INFO(p->log, "[{}] Starting Simulator\n", __func__);
        p->sim->simulate();
    }

}
