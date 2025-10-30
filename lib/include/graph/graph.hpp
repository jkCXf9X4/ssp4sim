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

        Graph(std::map<std::string, Invocable *> node_map, utils::DataRecorder *recorder)
        {
            this->recorder =recorder;
            this->node_map = node_map;
            nodes = utils::map_ns::map_to_value_vector_copy(node_map);
        }

        virtual void print(std::ostream &os) const
        {
            auto strong_system_graph = utils::graph::strongly_connected_components(utils::graph::Node::cast_to_parent_ptrs(nodes));

            os << "Simulation Graph DOT:\n"
               << utils::graph::Node::to_dot(nodes) << "\n"
               << utils::graph::ssc_to_string(strong_system_graph) << "\n";

            os << "node_map:\n";
            for (auto &[name, model] : node_map)
            {
                os << "Model: " << name << "\n";
            }
        }

        void init()
        {
            log(trace)("[{}] Initializing Graph", __func__);

            executor = ExecutorBuilder().build(nodes);
            executor->set_recorder(recorder);

            log(trace)("[{}] - Initializing executor ", __func__);
            executor->init();
        }

        // hot path
        uint64_t invoke(StepData step_data) override final
        {
            IF_LOG({
                log(trace)("[{}] Invoking Graph, full step: {}", __func__, step_data.to_string());
            });

            auto t = step_data.start_time;
            while (t < step_data.end_time)
            {
                auto s = StepData(t, t + step_data.timestep, step_data.timestep);

                IF_LOG({
                    log(debug)("[{}] Graph executing step: {}", __func__, s.to_string());
                });

                executor->invoke(s);

                t += step_data.timestep;

            }
            return t;
        }
    };

}