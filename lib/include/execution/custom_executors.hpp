#pragma once

#include "cutecpp/log.hpp"

#include "ssp4sim_definitions.hpp"

#include "executor.hpp"
#include "invocable.hpp"

#include "executor_utils.hpp"

#include "config.hpp"

#include <assert.h>
#include <execution>

namespace ssp4sim::graph
{

    static Invocable *node_from_name(ExecutionBase *executor, std::string name)
    {
        for (auto &node : executor->nodes)
        {
            if (node->name == name)
            {
                return node;
            }
        }
        executor->log(error)("[{}] In {} node: {} not found", __func__, executor->name, name);
        throw std::runtime_error("Node not found");
    }

    static void gauss_seidel(std::vector<Invocable *> &_nodes_, StepData &step_data, uint64_t sub_step)
    {
        // log(info)("New group");
        int accumulated_delay = 0;
        for (auto &node : _nodes_)
        {
            auto macro_start = step_data.start_time + accumulated_delay;
            auto macro_end = macro_start + step_data.timestep;

            auto s = StepData(macro_start, macro_end, sub_step);
            invoke_sub_step(node, s, true);

            accumulated_delay += node->delay;
        }
    }

    class DelayExecutor final : public ExecutionBase
    {
    public:
        Logger log = Logger("ssp4sim.execution.DelayExecutor", LogLevel::info);

        std::vector<Invocable *> g1;
        std::vector<Invocable *> g2;
        std::vector<Invocable *> g3;
        std::vector<Invocable *> g4;

        std::vector<std::vector<Invocable *>> groups;

        DelayExecutor(std::vector<Invocable *> nodes) : ExecutionBase(nodes)
        {
            this->name = "DelayExecutor";
            log(info)("[{}] substep: {}", __func__,  sub_step);

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
            auto step = StepData(step_data.start_time, step_data.end_time, step_data.timestep);

            IF_LOG({
                log(debug)("[{}] {} stepdata: {}", __func__, name, step_data.to_string());
            });

            std::for_each(std::execution::par, groups.begin(), groups.end(),
                          [&](auto &group)
                          {
                              gauss_seidel(group, step, sub_step);
                          });

            wait_for_result_collection();

            return step_data.end_time;
        }
    };

    class DelayExecutor2 final : public ExecutionBase
    {
    public:
        Logger log = Logger("ssp4sim.execution.DelayExecutor2", LogLevel::info);

        std::vector<Invocable *> g1;
        std::vector<Invocable *> g2;
        std::vector<Invocable *> g3;
        std::vector<Invocable *> g4;
        std::vector<Invocable *> g12;

        std::vector<std::vector<Invocable *>> groups;

        DelayExecutor2(std::vector<Invocable *> nodes) : ExecutionBase(nodes)
        {
            name = "DelayExecutor2";
            log(info)("[{}] substep: {}", __func__, sub_step);

            auto source = node_from_name(this, "Sources");
            auto let1 = node_from_name(this, "LET1");
            auto let2 = node_from_name(this, "LET2");
            auto let3 = node_from_name(this, "LET3");
            auto let4 = node_from_name(this, "LET4");
            auto let5 = node_from_name(this, "LET5");

            g1.push_back(source);
            g1.push_back(let1);
            g1.push_back(let2);
            g2.push_back(let3);
            g3.push_back(let4);
            g4.push_back(let5);

            groups.push_back(std::move(g1));
            groups.push_back(std::move(g2));
            groups.push_back(std::move(g3));
            groups.push_back(std::move(g4));
        }


        // hot path
        uint64_t invoke(StepData step_data) override final
        {
            auto step = StepData(step_data.start_time, step_data.end_time, step_data.timestep);

            IF_LOG({
                log(debug)("[{}] {} stepdata: {}", __func__, name, step_data.to_string());
            });

            std::for_each(std::execution::par, groups.begin(), groups.end(),
                          [&](auto &group)
                          {
                              gauss_seidel(group, step, sub_step);
                          });

            wait_for_result_collection();

            return step_data.end_time;
        }
    };
}
