#include "executor_builder.hpp"
#include "sim_setup.hpp"

#include "pre/3_simulation_graph/builder/sim_graph_builder.hpp"
#include "pre/3_simulation_graph/elements/model_fmu.hpp"
#include "simulation/signal/recorder.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ssp4sim::pre
{
    SimulationData setup_sim_behaviour(
        std::map<std::string, std::shared_ptr<graph::Invocable>> models,
        signal::DataRecorder *recorder,
        ssp4sim::SharedConfig *config)
    {
        (void)config;
        graph::GraphBuilder::register_model_storages(models, recorder);

        auto log = ssp4cpp::utils::log::make_logger("ssp4sim.pre.SimulationData");
        LOG_INFO(log, "[{func}] - Creating simulation graph executor", __func__);


        //TODO: setup_sim_behaviour should get SimulationGraph and then get the vector from there 
        std::vector<std::shared_ptr<graph::Invocable>> nodes;
        nodes.reserve(models.size());
        for (auto &[name, model] : models)
        {
            (void)name;
            nodes.push_back(std::move(model));
        }

        graph::ExecutorBuilder builder;
        SimulationData data;
        data.execution_node = builder.build(std::move(nodes));
        return data;
    }

} // namespace ssp4sim::pre