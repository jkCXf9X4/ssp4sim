#pragma once

#include "cutecpp/log.hpp"
#include "utils/map.hpp"
#include "utils/vector.hpp"
#include "utils/data_recorder.hpp"

#include "ssp4sim_definitions.hpp"

#include "invocable.hpp"
#include "jacobi.hpp"
#include "seidel.hpp"

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

        std::map<std::string, std::unique_ptr<Invocable>> models;
        std::vector<Invocable *> nodes;

        std::unique_ptr<ExecutionBase> executor;

        utils::DataRecorder *recorder = nullptr;
        const bool wait_for_recorder = utils::Config::getOr<bool>("simulation.ensure_results", false);

        Graph() = default;

        Graph(std::map<std::string, std::unique_ptr<Invocable>> models_)
            : models(std::move(models_))
        {
            auto m = utils::map_ns::map_unique_to_ref(models);
            nodes = utils::map_ns::map_to_value_vector_copy(m);
        }

        virtual void print(std::ostream &os) const
        {
            auto strong_system_graph = utils::graph::strongly_connected_components(utils::graph::Node::cast_to_parent_ptrs(nodes));

            os << "Simulation Graph DOT:\n"
               << utils::graph::Node::to_dot(nodes) << "\n"
               << utils::graph::ssc_to_string(strong_system_graph) << "\n";

            os << "Models:\n";
            for (auto &[name, model] : models)
            {
                os << "Model: " << name << "\n";
            }
        }

        void init()
        {
            log(trace)("[{}] Initializing Graph", __func__);

            executor = select_executor();

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

                if (wait_for_recorder)
                {
                    recorder->wait_until_done();
                }
            }
            return t;
        }

        std::unique_ptr<ExecutionBase> select_executor()
        {
            auto executor_method = utils::Config::getOr<std::string>("simulation.executor", "jacobi");
            if (executor_method == "jacobi")
            {
                if (utils::Config::getOr<bool>("simulation.jacobi.parallel", false))
                {

                    int parallel_method = utils::Config::getOr<int>("simulation.jacobi.method", 1);
                    int workers = utils::Config::getOr<int>("simulation.thread_pool_workers", 5);

                    if (parallel_method == 1)
                    {
                        log(info)("[{}] Running JacobiParallelTBB", __func__);
                        return std::make_unique<JacobiParallelTBB>(nodes);
                    }
                    else if (parallel_method == 2)
                    {
                        log(info)("[{}] Running JacobiParallelSpin", __func__);
                        return std::make_unique<JacobiParallelSpin>(nodes, workers);
                    }
                    else if (parallel_method == 3)
                    {
                        log(info)("[{}] Running JacobiParallelFutures", __func__);
                        return std::make_unique<JacobiParallelFutures>(nodes, workers);
                    }
                    else
                    {
                        throw std::runtime_error("Unknown parallelization method");
                    }
                }
                else
                {
                    log(info)("[{}] Running JacobiSerial", __func__);
                    return std::make_unique<JacobiSerial>(nodes);
                }
            }
            else if (executor_method == "seidel")
            {
                if (utils::Config::getOr<bool>("simulation.seidel.parallel", false))
                {
                    // int workers = utils::Config::getOr<int>("simulation.thread_pool_workers", 5);

                    log(info)("[{}] Running ParallelSeidel", __func__);
                    return std::make_unique<ParallelSeidel>(nodes);
                }
                else
                {
                    log(info)("[{}] Running SerialSeidel", __func__);
                    return std::make_unique<SerialSeidel>(nodes);
                }
            }
            else
            {
                throw std::runtime_error("Unknown execution method");
            }
        }
    };

}