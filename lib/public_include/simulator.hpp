#pragma once

#include <memory>
#include <string>


namespace ssp4sim
{
    struct SimulatorPrivate;

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

        /**
         * @brief Constructs a new Simulator object.
         *
         * @param config_path The path to the simulation configuration file.
         */
        explicit Simulator(const std::string config_path);

        Simulator(const Simulator &) = delete;
        Simulator &operator=(const Simulator &) = delete;

        Simulator(Simulator &&) = delete;
        Simulator &operator=(Simulator &&) = delete;

        ~Simulator();

        /**
         * @brief Initializes the simulator.
         */
        void init();

        /**
         * @brief Runs the simulation.
         */
        void simulate();

    private:
        std::unique_ptr<SimulatorPrivate> p;
    };
}
