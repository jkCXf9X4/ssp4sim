#pragma once

#include "utils/map.hpp"
#include "utils/vector.hpp"
#include "utils/time.hpp"

#include "invocable.hpp"
#include "analysis_graph_builder.hpp"
#include "graph_builder.hpp"

#include "fmu_handler.hpp"
#include "data_recorder.hpp"

#include "SSP_Ext.hpp"
#include "ssp4cpp/ssp.hpp"
#include "ssp4cpp/fmu.hpp"
#include "config.hpp"

#include <vector>
#include <algorithm>
#include <map>
#include <set>
#include <list>
#include <chrono>

namespace ssp4sim
{
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

        bool enable_recording = utils::Config::getOr<bool>("simulation.enable_recording", true);
        std::string result_file = utils::Config::get<std::string>("simulation.result_file");

        /**
         * @brief Constructs a new Simulation object.
         *
         * @param ssp A pointer to the SSP object to be simulated.
         */
        Simulation(ssp4cpp::Ssp *ssp)
        {
            this->ssp = ssp;

            log(info)("[{}] Creating simulation", __func__);
            fmu_handler = std::make_unique<handler::FmuHandler>(this->ssp);
            recorder = std::make_unique<utils::DataRecorder>(result_file);
        }

        /**
         * @brief Initializes the simulation.
         *
         */
        void init()
        {
            log(info)("[{}] Initializing simulation", __func__);

            log(info)("[{}] - Initializing fmus", __func__);
            fmu_handler->init();

            log(info)("[{}] - Creating analysis graph", __func__);
            auto analysis_graph = analysis::graph::AnalysisGraphBuilder(ssp, fmu_handler.get()).build();
            log(debug)(" -- {}", analysis_graph->to_string());

            log(info)("[{}] - Creating simulation graph", __func__);
            auto graph_builder = graph::GraphBuilder(analysis_graph.get(), recorder.get());
            graph_builder.build();

            this->sim_graph = graph_builder.get_graph();
            log(debug)(" -- {}", sim_graph->to_string());

            this->nodes = graph_builder.get_models(); // transfer ownership of nodes to simulation

            log(info)("[{}] - Init simulation graph", __func__);
            sim_graph->init();

            log(info)("[{}] - Initializing recorder", __func__);
            recorder->init();
        }

        /**
         * @brief Runs the simulation.
         *
         * This method runs the simulation from the configured start time to the
         * end time, with the specified timestep. The simulation results are
         * written to the configured output file.
         */
        void simulate()
        {
            if (enable_recording)
            {
                recorder->start_recording();
            }

            log(info)("[{}] Starting simulation", __func__);

            uint64_t start_time = utils::time::s_to_ns(utils::Config::get<double>("simulation.start_time"));
            uint64_t end_time = utils::time::s_to_ns(utils::Config::get<double>("simulation.stop_time"));
            uint64_t timestep = utils::time::s_to_ns(utils::Config::get<double>("simulation.timestep"));

            auto sim_timer = utils::time::Timer();

            sim_graph->invoke(ssp4sim::graph::StepData(start_time, end_time, timestep));
            
            auto sim_wall_time = sim_timer.stop();

            log(info)("[{}] Total walltime: {} ", __func__, utils::time::ns_to_s(sim_wall_time));

            recorder->stop_recording();
            log(info)("[{}] Simulation completed\n", __func__);

            uint64_t total_model_time = 0;
            for (auto &node : sim_graph->nodes)
            {
                auto model_walltime = node->walltime_ns;
                log(info)("[{}] Model {} walltime: {}", __func__, node->name, utils::time::ns_to_s(model_walltime));
                total_model_time += model_walltime;
            }
            log(info)("[{}] Model walltime: {}", __func__, utils::time::ns_to_s(total_model_time));
        }
    };

}