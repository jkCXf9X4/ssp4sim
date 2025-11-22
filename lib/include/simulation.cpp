
#include "simulation.hpp"

#include "utils/timer.hpp"

#include "analysis_graph_builder.hpp"
#include "graph_builder.hpp"

#include "data_recorder.hpp"

#include "config.hpp"

namespace ssp4sim
{

    Simulation::Simulation(ssp4cpp::Ssp *ssp)
    {
        this->ssp = ssp;

        log(info)("[{}] Creating simulation", __func__);
        fmu_handler = std::make_unique<handler::FmuHandler>(this->ssp);

        auto enable_recording = utils::Config::getOr("simulation.recording.enable", true);
        auto result_file = utils::Config::getOr("simulation.recording.result_file", std::string("./result/data.scv"));
        auto recording_interval = utils::time::s_to_ns(utils::Config::getOr("simulation.recording.interval", 1.0));
        auto wait_for_recorder = utils::Config::getOr("simulation.recording.wait_for", false);

        if (enable_recording)
        {
            recorder = std::make_unique<utils::DataRecorder>(result_file, recording_interval, wait_for_recorder);
        }

    }

    Simulation::~Simulation() = default;

    /**
     * @brief Initializes the simulation.
     *
     */
    void Simulation::init()
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
    void Simulation::simulate()
    {
        if (recorder)
        {
            recorder->start_recording();
        }

        log(info)("[{}] Starting simulation", __func__);

        uint64_t start_time = utils::time::s_to_ns(utils::Config::getDouble("simulation.start_time"));
        uint64_t end_time = utils::time::s_to_ns(utils::Config::getDouble("simulation.stop_time"));
        uint64_t timestep = utils::time::s_to_ns(utils::Config::getDouble("simulation.timestep"));

        auto sim_timer = utils::time::Timer();

        try
        {
            sim_graph->invoke(ssp4sim::graph::StepData(start_time, end_time, timestep));
        }
        catch (const std::runtime_error& e)
        {
            log(error)(std::format("Simulation failed! {} ", e.what()));
        }
        
        auto sim_wall_time = sim_timer.stop();

        log(info)("[{}] Total walltime: {} ", __func__, utils::time::ns_to_s(sim_wall_time));

        if (recorder)
        {
            recorder->stop_recording();
        }

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
}
