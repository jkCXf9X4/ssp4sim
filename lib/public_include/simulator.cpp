#include "simulator.hpp"

#include "config.hpp"
#include "simulation.hpp"
#include "ssp4cpp/ssp.hpp"

#include "cutecpp/log.hpp"

#include <filesystem>
#include <exception>

namespace ssp4sim
{
    struct SimulatorPrivate
    {
        Logger log = Logger("ssp4sim.Simulator", LogLevel::info);

        std::unique_ptr<ssp4cpp::Ssp> ssp;
        std::unique_ptr<Simulation> sim;
    };


    Simulator::Simulator(const std::string config_path)
        : p(std::make_unique<SimulatorPrivate>())
    {
        p->log(info)("[{}] Setting up Simulator", __func__);
        
        p->log(info)("[{}] - Loading config: {}", __func__, config_path);
        if (!std::filesystem::exists(config_path))
        {
            p->log(error)("Config file does not exist");
            return;
        }

        utils::Config::loadFromFile(config_path);
        p->log(debug)("[{}] - Config loaded:\n{}\n", __func__, utils::Config::as_string());

        auto log_file = utils::Config::getString("simulation.log.file");
        p->log.enable_file_sink(log_file, false);
        p->log(info)("[{}] - File log enabled, {}", __func__, log_file);

        try
        {
            p->log.enable_socket_sink("127.0.0.1:19996");
            p->log(info)("[{}] - Socket log enabled, use cutelog - 127.0.0.1:19996", __func__);
        }
        catch (const std::exception &)
        {
            p->log(warning)("[{}] - Socket log disabled", __func__);
        }

        p->log(debug)("[{}] - Importing SSP", __func__);
        auto ssp_path = utils::Config::getString("simulation.ssp");
        auto ssd = utils::Config::getOr("simulation.ssd", std::string("SystemStructure.ssd"));
        p->ssp = std::make_unique<ssp4cpp::Ssp>(ssp_path, ssd);
        p->log(debug)("[{}] -- SSP: {}", __func__, p->ssp->to_string());

        p->log(debug)("[{}] - Creating simulation\n", __func__);
        p->sim = std::make_unique<Simulation>(p->ssp.get());
    }

    Simulator::~Simulator() = default;

    void Simulator::init()
    {
        p->log(info)("[{}] Initializing Simulator\n", __func__);
        p->sim->init();
    }

    void Simulator::simulate()
    {
        p->log(info)("[{}] Starting Simulator\n", __func__);
        p->sim->simulate();
    }

}
