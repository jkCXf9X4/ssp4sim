#pragma once

#include "cutecpp/log.hpp"

#include <memory>
#include <string>

namespace ssp4cpp
{
    class Ssp; // Forward dec
}

namespace ssp4sim
{
    class Simulation; // Forward dec

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
        Logger log = Logger("ssp4sim.Simulator", LogLevel::info);

        std::unique_ptr<ssp4cpp::Ssp> ssp;
        std::unique_ptr<Simulation> sim;

        /**
         * @brief Constructs a new Simulator object.
         *
         * @param config_path The path to the simulation configuration file.
         */
        explicit Simulator(const std::string &config_path);

        ~Simulator();

        /**
         * @brief Initializes the simulator.
         */
        void init();

        /**
         * @brief Runs the simulation.
         */
        void simulate();

    };
}
