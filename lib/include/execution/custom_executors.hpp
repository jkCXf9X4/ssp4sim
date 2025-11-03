#pragma once

#include "cutecpp/log.hpp"

#include "ssp4sim_definitions.hpp"

#include "executor.hpp"
#include "invocable.hpp"

#include "config.hpp"

#include <assert.h>
#include <execution>

namespace ssp4sim::graph
{

    class DelayExecutor final : public ExecutionBase
    {
    public:
        Logger log = Logger("ssp4sim.execution.DelayExecutor", LogLevel::info);

        std::vector<Invocable *> g1;
        std::vector<Invocable *> g2;
        std::vector<Invocable *> g3;
        std::vector<Invocable *> g4;

        std::vector<std::vector<Invocable *>> groups;

        uint64_t substep = utils::time::s_to_ns(utils::Config::getOr<double>("simulation.executor.custom_delay.sub_step", 0.001));

        DelayExecutor(std::vector<Invocable *> nodes) : ExecutionBase(nodes)
        {
            log(info)("[{}] DelayExecutor, substep: {}", __func__, substep);

            auto source = node_from_name("Sources");
            auto let1 = node_from_name("LET1");
            auto let2 = node_from_name("LET2");
            auto let3 = node_from_name("LET3");
            auto let4 = node_from_name("LET4");
            auto let5 = node_from_name("LET5");

            auto c1 = node_from_name("C1");
            auto c2 = node_from_name("C2");
            auto c3 = node_from_name("C3");
            auto c4 = node_from_name("C4");

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

        virtual void print(std::ostream &os) const
        {
            os << "DelayExecutor:\n{}\n";
        }

        Invocable *node_from_name(std::string name)
        {
            for (auto &node : this->nodes)
            {
                if (node->name == name)
                {
                    return node;
                }
            }
            throw std::runtime_error("Node not found");
        }

        void gauss_seidel(std::vector<Invocable *> &_nodes_, StepData &step_data)
        {
            // log(info)("New group");
            int accumulated_delay = 0;
            for (auto &node : _nodes_)
            {
                auto macro_start = step_data.start_time + accumulated_delay;
                auto macro_end = macro_start + step_data.timestep;

                while (node->current_time < macro_end)
                {
                    auto substep_start = node->current_time;
                    auto substep_end = node->current_time + substep;

                    auto output = substep_start + node->delay > substep_end ? substep_start + node->delay : substep_end;

                    auto s = StepData(substep_start, // start
                                      substep_end,   // end
                                      substep,       // step_size
                                      substep_start, // input
                                      output);       // output
                    IF_LOG({
                        log(info)("Node {}, current {} accumulated_delay: {}, step: {}",
                                  node->name, node->current_time, accumulated_delay, s.to_string());
                    });

                    node->invoke(s);
                }
                accumulated_delay += node->delay;
            }
        }

        // hot path
        uint64_t invoke(StepData step_data) override final
        {
            auto step = StepData(step_data.start_time, step_data.end_time, step_data.timestep);

            IF_LOG({
                log(debug)("[{}] DelayExecutor stepdata: {}", __func__, step_data.to_string());
            });

            std::for_each(std::execution::par, groups.begin(), groups.end(),
                          [&](auto &group)
                          {
                              gauss_seidel(group, step);
                          });

            wait_for_result_collection();

            return step_data.end_time;
        }
    };
}
