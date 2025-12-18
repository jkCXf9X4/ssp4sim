#pragma once

#include <memory>

namespace ssp4cpp
{
    class Ssp;
}

namespace ssp4sim
{

    struct SimulationPrivate;

    /**
     * @brief Main class for running a simulation.
     *
     * This class orchestrates the entire simulation process, from loading the SSP
     * file to initializing the FMUs, building the simulation graph, and running
     * the simulation.
     */
    class Simulation
    {
    private:
        std::unique_ptr<SimulationPrivate> p;

    public:
        /**
         * @brief Constructs a new Simulation object.
         *
         * @param ssp A pointer to the SSP object to be simulated.
         */
        Simulation(ssp4cpp::Ssp *ssp);

        /**
         * @brief Initializes the simulation.
         *
         */
        void init();

        /**
         * @brief Runs the simulation.
         *
         * This method runs the simulation from the configured start time to the
         * end time, with the specified timestep. The simulation results are
         * written to the configured output file.
         */
        void simulate();

        ~Simulation();
    };

}
