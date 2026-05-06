
#include "simulation.hpp"

#include "utils/timer.hpp"

#include "analysis_graph_builder.hpp"
#include "graph_builder.hpp"

#include "signal/influx_recorder_sink.hpp"
#include "signal/csv_recorder_sink.hpp"
#include "signal/recorder.hpp"

#include "config.hpp"

#include "handler/fmu_handler.hpp"

#include "execution/invocable.hpp"
#include "graph/graph.hpp"

#include "ssp4cpp/utils/log.hpp"

#include "ssp4cpp/fmu.hpp"

#include "utils/io.hpp"

#include <cstdint>
#include <exception>
#include <filesystem>
#include <map>
#include <memory>
#include <stdexcept>

namespace ssp4sim
{

    struct SimulationPrivate
    {
        ssp4cpp::utils::log::Logger *log = ssp4cpp::utils::log::make_logger("ssp4sim.Simulation");

        ssp4cpp::Ssp *ssp;

        std::unique_ptr<handler::FmuHandler> fmu_handler;
        std::unique_ptr<signal::DataRecorder> recorder = nullptr;
        std::unique_ptr<graph::Graph> sim_graph;

        std::map<std::string, std::unique_ptr<graph::Invocable>> nodes;
    };

    Simulation::Simulation(ssp4cpp::Ssp *ssp, ssp4sim::SharedConfig *config) : p(std::make_unique<SimulationPrivate>())
    {
        p->ssp = ssp;
        this->config = config;

        LOG_INFO(p->log, "[{func}] Creating simulation", __func__);
        p->fmu_handler = std::make_unique<handler::FmuHandler>(p->ssp);

        if (config->enable_recording)
        {
            p->recorder = std::make_unique<signal::DataRecorder>(config->wait_for_recorder);

            if (config->influx.enable)
            {
                p->recorder->add_sink(std::make_unique<signal::InfluxRecorderSink>(config->influx));
            }

            if (config->csv.enable)
            {
                p->recorder->add_sink(std::make_unique<signal::CsvRecorderSink>(config->csv.file, config->recording_interval));
            }
        }
    }

    Simulation::~Simulation() = default;

    /**
     * @brief Initializes the simulation.
     *
     */
    void Simulation::init()
    {
        LOG_INFO(p->log, "[{func}] Initializing simulation", __func__);

        LOG_INFO(p->log, "[{func}] - Initializing fmus", __func__);
        p->fmu_handler->init();

        LOG_INFO(p->log, "[{func}] - Creating analysis graph", __func__);
        auto analysis_graph = analysis::graph::AnalysisGraphBuilder(p->ssp, p->fmu_handler.get()).build();
        LOG_DEBUG(p->log, " -- {graph}", analysis_graph->to_string());

        LOG_INFO(p->log, "[{func}] - Creating simulation graph", __func__);
        auto graph_builder = graph::GraphBuilder(analysis_graph.get(), p->recorder.get(), this->config);
        graph_builder.build();

        p->sim_graph = graph_builder.get_graph();
        LOG_DEBUG(p->log, " -- {graph}", p->sim_graph->to_string());

        p->nodes = graph_builder.get_models(); // transfer ownership of nodes to simulation

        LOG_INFO(p->log, "[{func}] - Init simulation graph", __func__);
        p->sim_graph->init();

        if (p->recorder)
        {
            LOG_INFO(p->log, "[{func}] - Initializing recorder", __func__);
            p->recorder->init();
        }
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
        if (p->recorder)
        {
            p->recorder->start_recording();
        }

        LOG_INFO(p->log, "[{func}] Starting simulation", __func__);

        auto sim_timer = utils::time::Timer();

        if (config->realtime)
        {
            p->sim_graph->enable_realtime(utils::time::time_now_ns());
        }

        std::exception_ptr simulation_error;

        try
        {
            p->sim_graph->invoke(ssp4sim::graph::StepData(config->start_time, config->end_time, config->timestep));
        }
        catch (const std::exception &e)
        {
            LOG_ERROR(p->log, "Simulation failed! {error}", e.what());
            simulation_error = std::current_exception();
        }
        catch (...)
        {
            LOG_ERROR(p->log, "Simulation failed! {error}", "Unknown error");
            simulation_error = std::current_exception();
        }

        auto sim_wall_time = sim_timer.stop();

        LOG_INFO(p->log, "[{func}] Total walltime: {walltime} ", __func__, utils::time::ns_to_s(sim_wall_time));

        if (p->recorder)
        {
            p->recorder->stop_recording();
        }

        if (simulation_error)
        {
            LOG_INFO(p->log, "[{func}] Simulation aborted\n", __func__);
        }
        else
        {
            LOG_INFO(p->log, "[{func}] Simulation completed\n", __func__);
        }

        uint64_t total_model_time = 0;
        for (auto &node : p->sim_graph->nodes)
        {
            auto model_walltime = node->walltime_ns;
            LOG_INFO(p->log, "[{func}] Model {model} walltime: {walltime}", __func__, node->name, utils::time::ns_to_s(model_walltime));
            total_model_time += model_walltime;
        }
        LOG_INFO(p->log, "[{func}] Model walltime: {walltime}", __func__, utils::time::ns_to_s(total_model_time));

        if (simulation_error)
        {
            std::rethrow_exception(simulation_error);
        }
    }
}
