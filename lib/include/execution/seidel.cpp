
#include "execution/seidel.hpp"

#include "cutecpp/log.hpp"

#include "ssp4sim_definitions.hpp"

#include "executor.hpp"

#include "invocable.hpp"
#include "task_thread_pool.hpp"
#include "task_thread_pool2.hpp"

#include "config.hpp"

#include <assert.h>
#include <execution>

namespace ssp4sim::graph
{

    SeidelBase::SeidelBase(std::vector<Invocable *> _nodes_) : ExecutionBase(std::move(_nodes_)), nr_of_nodes(nodes.size()), seidel_nodes(nr_of_nodes)
    {
        log(info)("[{}] ", __func__);
        log(trace)("[{}] nr_of_nodes {}, seidel_nodes {}", __func__, nr_of_nodes, seidel_nodes.size());

        for (auto &node : this->nodes)
        {
            auto id = node->id;

            auto &n = seidel_nodes[id];
            n.id = id;
            n.node = node;
            n.nr_parents = node->parents.size();
            n.nr_parents_counter = n.nr_parents;

            log(ext_trace)("[{}] Assigning SeidelNode {}", __func__, id);
        }

        log(info)("[{}] Evaluating start nodes", __func__);
        for (auto &node : this->seidel_nodes)
        {
            if (node.node->nr_parents() == 0)
            {
                start_nodes.push_back(&node);
            }
        }

        // break algebraic loops here?
        // Add a delay of a minor time amount to make sure that the broken loop cant use data that is to new
    }
        // continuous input will allow the substep to sample new data during the substep
    inline void invoke_sub_step(Invocable *node, const StepData &step_data, bool continuous_input = false)
    {
        while (node->current_time < step_data.end_time)
        {
            auto substep_start = node->current_time;
            auto substep_end = node->current_time + step_data.timestep;

            auto output_time = substep_end;
            if (node->delay == 0)
            {
                // No delay specified, just set it at the end
                output_time = substep_end;
            }
            else if (substep_start + node->delay <= substep_end)
            {
                // if the step is shorter than the model delay, do the best of it and set it to sub_step_end
                // evaluate if this is true, it could be set to the correct time but there is the potential
                // that the data could be used non-deterministic if a time before substep_end is set...
                output_time = substep_end;
            }
            else if (substep_start + node->delay > substep_end)
            {
                // if the step is longer than the model delay, set the correct time
                output_time = substep_start + node->delay;
            }

            // the valid input time does not work for seidel or anything that is not limited to the start time for valid inputs
            auto valid_input_time = continuous_input ? substep_start : step_data.input_time;
            auto s = StepData(substep_start,      // start
                              substep_end,        // end
                              step_data.timestep, // step_size
                              substep_start,   // input
                              output_time);       // output_time

            IF_LOG({
                node->log(info)("Node {}, Time {}, step: {}",
                          node->name, node->current_time, s.to_string());
            });

            node->invoke(s);
        }
    }

    void SeidelBase::reset_counters()
    {
        for (auto &n : seidel_nodes)
        {
            n.nr_parents_counter = n.nr_parents;
            n.invoked = false;
        }
    }

    SerialSeidel::SerialSeidel(std::vector<Invocable *> nodes) : SeidelBase(nodes)
    {
        log(info)("[{}] ", __func__);
    }

    /**
     * Traverse the connection graph and invoke nodes when all parents have been invoked for this timestep.
     * [hot path]
     */
    uint64_t SerialSeidel::invoke(StepData step_data)
    {
        IF_LOG({
            log(trace)("[{}] step data: {}", __func__, step_data.to_string());
        });

        int completed = 0;
        reset_counters();

        IF_LOG({
            log(trace)("[{}] Invoking nodes", __func__);
        });

        auto s = StepData(step_data.start_time, // start
                          step_data.end_time,   // end
                          sub_step,             // step_size
                          step_data.end_time,   // it should be able to use results from the current iteration
                          step_data.end_time);  // output_time

        while (nr_of_nodes != completed)
        {
            for (auto &node : seidel_nodes)
            {
                if (node.nr_parents_counter == 0 && !node.invoked)
                {

                    IF_LOG({
                        log(trace)("[{}] Starting {}:{}", __func__, node.id, node.node->name);
                    });

                    // node.node->invoke(s);
                    invoke_sub_step(node.node, s, true);

                    node.invoked = true;
                    completed++;
                    for (auto c : node.node->children)
                    {
                        auto &child = seidel_nodes[((Invocable *)c)->id];
                        child.nr_parents_counter -= 1;
                    }
                }
            }
        }

        IF_LOG({
            log(trace)("[{}] End. completed  {}", __func__, completed);
        });

        wait_for_result_collection();

        return step_data.end_time;
    }

    ParallelSeidel::ParallelSeidel(std::vector<Invocable *> nodes) : SeidelBase(nodes)
    {
        log(info)("[{}]", __func__);
    }

    /**
     * Traverse the connection graph and invoke nodes when all parents have been invoked for this timestep.
     * [hot path]
     */
    uint64_t ParallelSeidel::invoke(StepData step_data)
    {
        IF_LOG({
            log(ext_trace)("[{}] step data: {}", __func__, step_data.to_string());
        });

        step_data.input_time = step_data.end_time;

        throw std::runtime_error("This is not implemented");

        // reset_counters();

        // // track how many are currently running
        // int launched = 0;
        // int completed = 0;

        // IF_LOG({
        //     log(trace)("[{}] Invoking all start nodes", __func__);
        // });

        // for (auto &sn : start_nodes)
        // {
        //     sn->node->async_invoke(step_data);
        //     launched += 1;
        // }

        // while (launched != completed)
        // {
        //     IF_LOG({
        //         log(trace)("[{}] Waiting for nodes to finish. launched {}, completed  {}", __func__, launched, completed);
        //     });

        //     shared_state->sem.acquire();
        //     completed += 1;

        //     DoneMsg msg;
        //     {
        //         std::lock_guard<std::mutex> lock(shared_state->mtx);
        //         msg = std::move(shared_state->inbox.front());
        //         shared_state->inbox.pop();
        //     }
        //     auto &finished_node = seidel_nodes[msg.worker_id];

        //     IF_LOG({
        //         log(trace)("[{}] Node finished: {}", __func__, finished_node.node->name);
        //     });

        //     // enqueue children whose all parents are invoked
        //     for (auto c : finished_node.node->children)
        //     {
        //         auto &child = seidel_nodes[((AsyncNode *)c)->worker_id];

        //         child.nr_parents_counter -= 1;
        //         if (child.nr_parents_counter == 0)
        //         {
        //             IF_LOG({
        //                 log(debug)("[{}] Node ready, invoking: {}", __func__, child.node->name);
        //             });

        //             child.node->async_invoke(step_data);
        //             launched += 1;
        //         }
        //     }
        // }
        // IF_LOG({
        //     log(trace)("[{}] End. launched {}, completed  {}", __func__, launched, completed);
        // });

        wait_for_result_collection();

        return step_data.end_time;
    }

}
