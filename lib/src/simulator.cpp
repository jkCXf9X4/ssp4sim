#include "simulator.hpp"

#include "config.hpp"
#include "simulation.hpp"
#include "ssp4cpp/ssp.hpp"

#include <exception>

namespace ssp4sim
{

    Simulator::Simulator(const std::string &config_path)
    {
        log(info)("[{}] Creating Simulator\n", __func__);

        log(debug)("[{}] - Loading config", __func__);
        utils::Config::loadFromFile(config_path);
        log(debug)("[{}] -- Config:\n{}\n", __func__, utils::Config::as_string());

        auto log_file = utils::Config::get<std::string>("simulation.log.file");
        log.enable_file_sink(log_file, false);
        log(info)("[{}] File log enabled, {}", __func__, log_file);

        try
        {
            log.enable_socket_sink("127.0.0.1:19996");
            log(info)("[{}] Socket log enabled", __func__);
        }
        catch (const std::exception &)
        {
            log(warning)("[{}] Socket log disabled", __func__);
        }

        log(debug)("[{}] - Importing SSP", __func__);
        auto ssp_path = utils::Config::get<std::string>("simulation.ssp");
        auto ssd = utils::Config::getOr<std::string>("simulation.ssd", "SystemStructure.ssd");
        ssp = std::make_unique<ssp4cpp::Ssp>(ssp_path, ssd);
        log(debug)("[{}] -- SSP: {}", __func__, ssp->to_string());

        log(debug)("[{}] - Creating simulation\n", __func__);
        sim = std::make_unique<Simulation>(ssp.get());
    }

    Simulator::~Simulator() = default;

    void Simulator::init()
    {
        log(info)("[{}] Initializing Simulator\n", __func__);
        sim->init();
    }

    void Simulator::simulate()
    {
        log(info)("[{}] Starting Simulator\n", __func__);
        sim->simulate();
    }

}
