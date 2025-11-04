#pragma once

#include "cutecpp/log.hpp"
#include "utils/map.hpp"
#include "utils/vector.hpp"
#include "utils/data_recorder.hpp"

#include "ssp4sim_definitions.hpp"

#include "invocable.hpp"
#include "executor.hpp"
#include "executor_builder.hpp"

#include "config.hpp"

#include <vector>
#include <algorithm>
#include <map>

namespace ssp4sim::graph
{

    class Graph final : public Invocable
    {
    public:
        Logger log = Logger("ssp4sim.graph.Graph", LogLevel::info);

        std::map<std::string, Invocable*> node_map;
        std::vector<Invocable *> nodes;

        std::unique_ptr<ExecutionBase> executor;
        utils::DataRecorder *recorder = nullptr;

        Graph() = default;

        Graph(std::map<std::string, Invocable *> node_map, utils::DataRecorder *recorder);

        void print(std::ostream &os) const override;

        void init();

        uint64_t invoke(StepData step_data) override final;
    };

}
