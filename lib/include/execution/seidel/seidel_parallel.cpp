
#include "execution/seidel/seidel_parallel.hpp"

#include "config.hpp"

#include <stdexcept>

namespace ssp4sim::graph
{
    ParallelSeidel::ParallelSeidel(std::vector<Invocable *> nodes)
        : SeidelBase(nodes),
          log(ssp4cpp::utils::log::make_logger("ssp4sim.execution.ParallelSeidel"))
    {
        LOG_INFO(log, "[{func}]", __func__);
    }

    /**
     * Traverse the connection graph and invoke nodes when all parents have been invoked for this timestep.
     * [hot path]
     */
    uint64_t ParallelSeidel::invoke(StepData step_data)
    {
        IF_LOG({
            LOG_TRACE_L1(log, "[{func}] step data: {}", __func__, step_data.to_string());
        });

        step_data.input_time = step_data.end_time;

        throw std::runtime_error("This is not implemented");

        // reset_counters();

        // // track how many are currently running
        // int launched = 0;
        // int completed = 0;

        // IF_LOG({
        //     LOG_TRACE_L1(log, "[{func}] Invoking all start nodes", __func__);
        // });

        // for (auto &sn : start_nodes)
        // {
        //     sn->node->async_invoke(step_data);
        //     launched += 1;
        // }

        // while (launched != completed)
        // {
        //     IF_LOG({
        //         LOG_TRACE_L1(log, "[{func}] Waiting for nodes to finish. launched {}, completed  {}", __func__, launched, completed);
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
        //         LOG_TRACE_L1(log, "[{func}] Node finished: {}", __func__, finished_node.node->name);
        //     });

        //     // enqueue children whose all parents are invoked
        //     for (auto c : finished_node.node->children)
        //     {
        //         auto &child = seidel_nodes[((AsyncNode *)c)->worker_id];

        //         child.nr_parents_counter -= 1;
        //         if (child.nr_parents_counter == 0)
        //         {
        //             IF_LOG({
        //                 LOG_DEBUG(log, "[{func}] Node ready, invoking: {}", __func__, child.node->name);
        //             });

        //             child.node->async_invoke(step_data);
        //             launched += 1;
        //         }
        //     }
        // }
        // IF_LOG({
        //     LOG_TRACE_L1(log, "[{func}] End. launched {}, completed  {}", __func__, launched, completed);
        // });

        return step_data.end_time;
    }

}
