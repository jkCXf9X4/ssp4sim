#pragma once

#include "cutecpp/log.hpp"

#include <map>
#include <memory>
#include <string>

namespace ssp4cpp
{
    class Ssp;
}

namespace ssp4sim
{
    namespace handler
    {
        class FmuHandler;
    }

    namespace utils
    {
        class DataRecorder;
    }

    namespace graph
    {
        class Graph;
        class Invocable;
    }

    /**
     * @brief Main class for running a simulation.
     *
     * This class orchestrates the entire simulation process, from loading the SSP
     * file to initializing the FMUs, building the simulation graph, and running
     * the simulation.
     */
    class Simulation
    {
    public:
        Logger log = Logger("ssp4sim.Simulation", LogLevel::info);

        ssp4cpp::Ssp *ssp;

        std::unique_ptr<handler::FmuHandler> fmu_handler;
        std::unique_ptr<utils::DataRecorder> recorder;
        std::unique_ptr<graph::Graph> sim_graph;

        std::map<std::string, std::unique_ptr<graph::Invocable>> nodes;

        bool enable_recording = true;
        std::string result_file = "";

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
