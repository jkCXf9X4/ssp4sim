#pragma once

#include "cutecpp/log.hpp"
#include "ssp4sim_definitions.hpp"

#include "invocable.hpp"
#include "executor.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ssp4sim::utils
{
    class DataRecorder;
}

namespace ssp4sim::graph
{
    class Graph final : public Invocable
    {
    public:
        Logger log = Logger("ssp4sim.graph.Graph", LogLevel::info);

        std::map<std::string, Invocable*> node_map;
        std::vector<Invocable *> nodes;

        std::unique_ptr<ExecutionBase> executor;
        ssp4sim::signal::DataRecorder *recorder = nullptr;

        Graph() = default;

        Graph(std::map<std::string, Invocable *> node_map, ssp4sim::signal::DataRecorder *recorder);

        void print(std::ostream &os) const override;

        void init();

        uint64_t invoke(StepData step_data) override final;
    };

}
