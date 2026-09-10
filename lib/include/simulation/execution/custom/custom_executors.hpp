#pragma once

#include "ssp4cpp/utils/log.hpp"

#include "ssp4sim_definitions.hpp"

#include "executor.hpp"
#include "invocable.hpp"

#include "config.hpp"

#include "pre/3_simulation_graph/elements/model_fmu.hpp"

#include "utils/time/time.hpp"

#include <assert.h>
#include <algorithm>
#include <cstdint>
#include <execution>
#include <stdexcept>
#include <string>
#include <vector>

namespace ssp4sim::graph
{

    class DelayExecutorBase : public ExecutionBase
    {
    public:
        std::vector<std::vector<Invocable *>> groups;

        DelayExecutorBase(std::vector<std::shared_ptr<Invocable>> nodes)
            : ExecutionBase(nodes, "ssp4sim.execution.DelayExecutor")
        {
            this->name = "DelayExecutor";
            LOG_INFO(log, "[{func}] ", __func__);
        }

        Invocable *node_from_name(ExecutionBase *executor, std::string name)
        {
            for (auto &node : executor->nodes)
            {
                if (node->name == name)
                {
                    return node.get();
                }
            }
            LOG_ERROR(executor->log, "[{func}] In {node} node: {name} not found", __func__, executor->name, name);
            throw std::runtime_error("Node not found");
        }

        // continuous input will allow the substep to sample new data during the substep
        void invoke_sub_step(FmuModel *models, const StepData &step_data, uint64_t substep_size)
        {
            while (models->current_time < step_data.end_time)
            {
                auto substep_start = models->current_time;
                auto substep_end = std::min(models->current_time + substep_size, step_data.end_time);

                auto s = StepData(substep_start, substep_end);

                IF_LOG({
                    LOG_TRACE_L1(log, "models {}, Time {}, step: {}",
                                 models->name, models->current_time, s.to_string());
                });

                models->invoke(s);
            }
        }

        void gauss_seidel(std::vector<Invocable *> &_nodes_, StepData &step_data, uint64_t substep_size)
        {
            IF_LOG({
                LOG_TRACE_L1(log, "New group");
            });
            for (auto &node : _nodes_)
            {
                auto model = (FmuModel *)node;
                invoke_sub_step(model, step_data, substep_size);
            }
        }
    };

    class DelayExecutor final : public DelayExecutorBase
    {
    public:
        std::vector<Invocable *> g1;
        std::vector<Invocable *> g2;
        std::vector<Invocable *> g3;
        std::vector<Invocable *> g4;

        DelayExecutor(std::vector<std::shared_ptr<Invocable>> nodes) : DelayExecutorBase(nodes)
        {
            this->name = "DelayExecutor";

            auto source = node_from_name(this, "Sources");
            auto let1 = node_from_name(this, "LET1");
            auto let2 = node_from_name(this, "LET2");
            auto let3 = node_from_name(this, "LET3");
            auto let4 = node_from_name(this, "LET4");
            auto let5 = node_from_name(this, "LET5");

            auto c1 = node_from_name(this, "C1");
            auto c2 = node_from_name(this, "C2");
            auto c3 = node_from_name(this, "C3");
            auto c4 = node_from_name(this, "C4");

            g1.push_back(source);
            g1.push_back(let1);
            g1.push_back(c1);
            g1.push_back(let2);
            g1.push_back(c2);

            g2.push_back(let3);
            g2.push_back(c3);

            g3.push_back(let4);
            g3.push_back(c4);

            g4.push_back(let5);

            groups.push_back(std::move(g1));
            groups.push_back(std::move(g2));
            groups.push_back(std::move(g3));
            groups.push_back(std::move(g4));
        }

        // hot path
        uint64_t invoke(StepData step_data) override final
        {
            auto step = StepData(step_data.start_time, step_data.end_time);

            IF_LOG({
                LOG_DEBUG(log, "[{func}] {name} stepdata: {stepdata}", __func__, name, step_data.to_string());
            });

            std::for_each(std::execution::par, groups.begin(), groups.end(),
                          [&](auto &group)
                          {
                              gauss_seidel(group, step, step.timestep);
                          });

            return step_data.end_time;
        }
    };

    class DelayExecutorPartial final : public DelayExecutorBase
    {
    public:
        std::vector<Invocable *> g1;
        std::vector<Invocable *> g2;
        std::vector<Invocable *> g3;
        std::vector<Invocable *> g4;
        std::vector<Invocable *> g12;

        DelayExecutorPartial(std::vector<std::shared_ptr<Invocable>> nodes) : DelayExecutorBase(nodes)
        {
            name = "DelayExecutorPartial";

            auto source = node_from_name(this, "Sources");
            auto let1 = node_from_name(this, "LET1");
            auto let2 = node_from_name(this, "LET2");
            auto let3 = node_from_name(this, "LET3");
            auto let4 = node_from_name(this, "LET4");
            auto let5 = node_from_name(this, "LET5");

            auto c1 = node_from_name(this, "C1");
            auto c2 = node_from_name(this, "C2");
            auto c3 = node_from_name(this, "C3");
            auto c4 = node_from_name(this, "C4");

            g1.push_back(source);
            g1.push_back(let1);
            g1.push_back(c1);
            g1.push_back(let2);
            g1.push_back(c2);

            g2.push_back(let3);
            g2.push_back(c3);

            g3.push_back(let4);
            g3.push_back(c4);

            g4.push_back(let5);
        }

        // hot path
        uint64_t invoke(StepData step_data) override final
        {
            auto step = StepData(step_data.start_time, step_data.end_time);

            IF_LOG({
                LOG_DEBUG(log, "[{func}] {name} stepdata: {stepdata}", __func__, name, step_data.to_string());
            });

            auto one_ms = utils::time::nanoseconds_per_millisecond;
            auto substep_size = step_data.timestep;

            auto s1 = StepData(step_data.start_time, step_data.start_time + 2 * one_ms);
            
            gauss_seidel(g1, s1, substep_size);
            gauss_seidel(g2, s1, substep_size);
            gauss_seidel(g3, s1, substep_size);
            
            
            auto s2 = StepData(step_data.start_time + 2 * one_ms, step_data.start_time + 4 * one_ms);
            gauss_seidel(g1, s2, substep_size);
            gauss_seidel(g2, s2, substep_size);
            gauss_seidel(g3, s2, substep_size);

            auto s3 = StepData(step_data.start_time, step_data.start_time + 4 * one_ms);
            gauss_seidel(g4, s3, substep_size);

            return step_data.end_time;
        }
    };
}