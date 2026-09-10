
#include "simulation.hpp"

#include "utils/time/timer.hpp"

#include "pre/pre_pipeline.hpp"

#include "simulation/signal/sinks/csv_recorder_sink.hpp"

#include "simulation/signal/sinks/sqlite_recorder_sink.hpp"
#include "simulation/signal/recorder.hpp"

#include "utils/config.hpp"

#include "ssp4cpp/utils/log.hpp"

#include "ssp4cpp/fmu.hpp"

#include "utils/io/io.hpp"
#include "utils/primitives/map.hpp"
#include "utils/primitives/uuid.hpp"

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

        std::string session_uuid;

        pre::SimulationGraph sim_graph; // model graph
        pre::SimulationData sim; // executors, access rules

        std::unique_ptr<signal::DataRecorder> recorder = nullptr;

        std::shared_ptr<Invocable> simulation_node;
    };

    Simulation::Simulation(ssp4cpp::Ssp *ssp, ssp4sim::SharedConfig *config) : p(std::make_unique<SimulationPrivate>())
    {
        p->ssp = ssp;
        this->config = config;
        p->session_uuid = utils::make_uuid_v4();

        LOG_INFO(p->log, "[{func}] Creating simulation", __func__);

        if (config->enable_recording)
        {
            p->recorder = std::make_unique<signal::DataRecorder>(config->wait_for_recorder);

            if (config->csv.enable)
            {
                p->recorder->add_sink(std::make_unique<signal::CsvRecorderSink>(config->csv.file, config->csv.interval));
            }

            if (config->sqlite.enable)
            {
                p->recorder->add_sink(std::make_unique<signal::SqliteWALRecorderSink>(config->working_dir, p->session_uuid, config->sqlite.file));
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
        if (p->simulation_node)
        {
            throw std::logic_error("Simulation::init() called twice");
        }

        LOG_INFO(p->log, "[{func}] Initializing simulation", __func__);

        p->sim_graph = pre::build_simulation_graph(p->ssp, this->config);

        // set up executors, data access rules, data recording mechanisms
        p->sim = sim_setup::setup_simulation(p->sim_graph->get_models(),  p->recorder.get(), this->config);
        p->simulation_node = p->sim->execution_nod;


        LOG_INFO(p->log, "[{func}] - Init simulation graph", __func__);
        p->simulation_node->init();

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
        if (!p->simulation_node)
        {
            throw std::runtime_error("Simulation::simulate() called before init()");
        }

        if (p->recorder)
        {
            p->recorder->start_recording();
        }

        LOG_INFO(p->log, "[{func}] Starting simulation", __func__);

        auto sim_timer = utils::time::Timer();

        std::exception_ptr simulation_error;

        try
        {
            p->simulation_node->invoke(ssp4sim::graph::StepData(config->start_time, config->end_time));
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
        for (auto& [key, node] : p->sim_graph.models)
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
