#include "graph/graph.hpp"

#include "execution/executor.hpp"
#include "execution/executor_builder.hpp"
#include "graph/graph_builder.hpp"
#include "utils/map.hpp"
#include "utils/time.hpp"
#include "signal/recorder.hpp"

#include "tarjan.hpp"

#include <sstream>
#include <thread>
#include <chrono>

namespace ssp4sim::graph
{

    Graph::Graph(std::map<std::string, Invocable *> node_map, ssp4sim::signal::DataRecorder *recorder)
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.graph.Graph", quill::LogLevel::TraceL1))
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
        LOG_DEBUG(log, "[{}] Initializing Graph", __func__);

        executor = ExecutorBuilder().build(nodes);
        executor->set_recorder(recorder);

        LOG_DEBUG(log, "[{}] - Initializing executor ", __func__);
        executor->init();
    }

    uint64_t Graph::invoke(StepData step_data)
    {
        using clock = std::chrono::steady_clock;
        IF_LOG({
            LOG_DEBUG(log, "[{}] Invoking Graph, full step: {}", __func__, step_data.to_string());
        });

        // macro step
        // ponder if this should be included in the executor...
        // will most likely be reused by several so might be placed as utils

        auto t = step_data.start_time;
        while (t < step_data.end_time)
        {
            if (realtime)
            {
                auto target = clock::time_point(std::chrono::nanoseconds(realtime_start_reference + t));
                std::this_thread::sleep_until(target);
                LOG_INFO(log, "[{}] Realtime: {}", __func__, t);
            }

            auto s = StepData(t, t + step_data.timestep, step_data.timestep);

            IF_LOG({
                LOG_TRACE_L1(log, "[{}] Graph executing step: {}", __func__, s.to_string());
            });

            executor->invoke(s);

            t += step_data.timestep;
        }

        return t;
    }

}
