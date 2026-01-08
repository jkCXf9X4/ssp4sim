#include "graph/graph.hpp"

#include "execution/executor.hpp"
#include "execution/executor_builder.hpp"
#include "graph/graph_builder.hpp"
#include "utils/map.hpp"
#include "signal/recorder.hpp"

#include "tarjan.hpp"

#include <sstream>

namespace ssp4sim::graph
{

    Graph::Graph(std::map<std::string, Invocable *> node_map, ssp4sim::signal::DataRecorder *recorder)
    {
        this->recorder = recorder;
        this->node_map = node_map;
        nodes = ssp4sim::utils::map_ns::map_to_value_vector_copy(this->node_map);
    }

    std::string Graph::to_string() const
    {
        auto strong_system_graph = ssp4sim::utils::graph::strongly_connected_components(ssp4sim::utils::graph::Node::cast_to_parent_ptrs(nodes));

        std::ostringstream oss;
        oss << "Simulation Graph DOT:\n"
            << ssp4sim::utils::graph::Node::to_dot(nodes) << "\n"
            << ssp4sim::utils::graph::ssc_to_string(strong_system_graph) << "\n";

        oss << "node_map:\n";
        for (auto &[name, model] : node_map)
        {
            oss << "Model: " << name << "\n";
        }
        return oss.str();
    }

    void Graph::init()
    {
        log(trace)("[{}] Initializing Graph", __func__);

        executor = ExecutorBuilder().build(nodes);
        executor->set_recorder(recorder);

        log(trace)("[{}] - Initializing executor ", __func__);
        executor->init();
    }

    uint64_t Graph::invoke(StepData step_data)
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

}
