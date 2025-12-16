#pragma once

#include "cutecpp/log.hpp"

#include "ssp4sim_definitions.hpp"

#include "executor.hpp"
#include "invocable.hpp"

#include "config.hpp"

#include "model/model_fmu.hpp"

#include "utils/time.hpp"

#include <assert.h>
#include <execution>

namespace ssp4sim::graph
{

    class DelayExecutorBase : public ExecutionBase
    {
    public:
        Logger log = Logger("ssp4sim.execution.DelayExecutor", LogLevel::info);

        std::vector<std::vector<Invocable *>> groups;

        DelayExecutorBase(std::vector<Invocable *> nodes) : ExecutionBase(nodes)
        {
            this->name = "DelayExecutor";
            log(info)("[{}] substep: {}", __func__, sub_step);
        }

        Invocable *node_from_name(ExecutionBase *executor, std::string name)
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

        // continuous input will allow the substep to sample new data during the substep
        void invoke_sub_step(FmuModel *models, const StepData &step_data)
        {
            while (models->current_time < step_data.end_time)
            {
                auto substep_start = models->current_time;
                auto substep_end = models->current_time + step_data.timestep;

                // NOTE: This probably does not work atm 
                auto output_time = substep_end;
                if (models->delay == 0)
                {
                    // No delay specified, just set it at the end
                    output_time = substep_end;
                }
                else if (substep_start + models->delay <= substep_end)
                {
                    // if the step is shorter than the model delay, do the best of it and set it to sub_step_end
                    // evaluate if this is true, it could be set to the correct time but there is the potential
                    // that the data could be used non-deterministic if a time before substep_end is set...
                    output_time = substep_end;
                }
                else if (substep_start + models->delay > substep_end)
                {
                    // if the step is longer than the model delay, set the correct time
                    output_time = substep_start + models->delay;
                }

                auto s = StepData(substep_start,      // start
                                  substep_end,        // end
                                  step_data.timestep, // step_size
                                  substep_start,      // input
                                  output_time);       // output_time

                IF_LOG({
                    log(info)("models {}, Time {}, step: {}",
                              models->name, models->current_time, s.to_string());
                });

                models->invoke(s);
            }
        }

        void gauss_seidel(std::vector<Invocable *> &_nodes_, StepData &step_data, uint64_t sub_step, int delay = 0)
        {
            log(info)("New group");
            int accumulated_delay = delay;
            for (auto &node : _nodes_)
            {
                auto model = (FmuModel *)node;
                log(warning)("accumulated_delay {}", accumulated_delay);
                auto macro_start = step_data.start_time + accumulated_delay;
                auto macro_end = macro_start + step_data.timestep;

                auto s = StepData(macro_start, macro_end, sub_step, macro_end, macro_end);
                log(warning)("Invoking node {}, {}", model->name, s.to_string());

                invoke_sub_step(model, s);
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

        DelayExecutor(std::vector<Invocable *> nodes) : DelayExecutorBase(nodes)
        {
            this->name = "DelayExecutor";
            log(info)("[{}] substep: {}", __func__, sub_step);

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

    class DelayExecutorPartial final : public DelayExecutorBase
    {
    public:
        std::vector<Invocable *> g1;
        std::vector<Invocable *> g2;
        std::vector<Invocable *> g3;
        std::vector<Invocable *> g4;
        std::vector<Invocable *> g12;

        DelayExecutorPartial(std::vector<Invocable *> nodes) : DelayExecutorBase(nodes)
        {
            name = "DelayExecutorPartial";
            log(info)("[{}] substep: {}", __func__, sub_step);

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
            auto step = StepData(step_data.start_time, step_data.end_time, step_data.timestep);

            IF_LOG({
                log(debug)("[{}] {} stepdata: {}", __func__, name, step_data.to_string());
            });

            auto one_ms = utils::time::nanoseconds_per_millisecond;

            auto s1 = StepData(step_data.start_time, step_data.start_time + 2 * one_ms, step_data.timestep);
            
            gauss_seidel(g1, s1, sub_step);
            gauss_seidel(g2, s1, sub_step, 2*one_ms);
            gauss_seidel(g3, s1, sub_step, 4*one_ms);
            
            
            auto s2 = StepData(step_data.start_time + 2 * one_ms, step_data.start_time + 4 * one_ms, step_data.timestep);
            gauss_seidel(g1, s2, sub_step);
            gauss_seidel(g2, s2, sub_step, 2*one_ms);
            gauss_seidel(g3, s2, sub_step, 4*one_ms);

            auto s3 = StepData(step_data.start_time, step_data.start_time + 4 * one_ms, step_data.timestep);
            gauss_seidel(g4, s3, sub_step, 8*one_ms);

            wait_for_result_collection();

            // throw std::runtime_error("Hello");
            return step_data.end_time;
        }
    };
}
