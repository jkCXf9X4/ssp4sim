#include "ssp.hpp"
#include "fmu.hpp"
#include "utils/io.hpp"
#include "utils/log.hpp"
#include "utils/json.hpp"
#include "utils/map.hpp"

#include "simulation.hpp"
#include "config.hpp"

#include <vector>
#include <map>

namespace ssp4sim::sim
{
    /**
     * @brief A high-level simulator class that simplifies the process of running
     * a simulation.
     *
     * This class provides a simple interface for loading an SSP file, configuring
     * the simulation, and running it. It handles the creation and management of
     * the underlying Simulation object.
     */
    class Simulator
    {
    public:
        common::Logger log = common::Logger("ssp4sim.Simulator", common::LogLevel::info);

        std::unique_ptr<Ssp> ssp;
        std::unique_ptr<Simulation> sim;

        /**
         * @brief Constructs a new Simulator object.
         *
         * @param config_path The path to the simulation configuration file.
         */
        Simulator(const std::string &config_path)
        {
            log.info("[{}] Creating Simulator\n", __func__);
            
            log.debug("[{}] - Loading config", __func__);
            utils::Config::loadFromFile(config_path);
            log.debug("[{}] -- Config:\n{}\n", __func__, utils::Config::as_string());

            auto log_file = "./results/log.log";
            log.enable_file_sink(log_file, false);
            log.info("[{}] File log enabled, {}", __func__, log_file);
            
            try
            {
                log.enable_socket_sink("127.0.0.1:19996");
                log.info("[{}] Socket log enabled", __func__);
            }
            catch(const std::exception& e)
            {
                log.warning("[{}] Socket log disabled", __func__);
            }

            log.debug("[{}] - Importing SSP", __func__);
            auto ssp_path = utils::Config::get<std::string>("simulation.ssp");
            ssp = std::make_unique<ssp4sim::Ssp>(ssp_path);
            log.debug("[{}] -- SSP: {}", __func__, ssp->to_string());
            
            log.debug("[{}] - Creating simulation\n", __func__);
            sim = std::make_unique<Simulation>(ssp.get());
        }
        
        ~Simulator()
        {
            // std::unique_ptr will automatically clean up FMUs and SSP
            // when this is destroyed the application will end, no need to free memory resources
        }
        
        /**
         * @brief Initializes the simulator.
         */
        void init()
        {
            log.info("[{}] Initializing Simulator\n", __func__);
            sim->init();
        }
        
        /**
         * @brief Runs the simulation.
         */
        void simulate()
        {
            log.info("[{}] Starting Simulator\n", __func__);
            sim->simulate();
        }


    };
}

