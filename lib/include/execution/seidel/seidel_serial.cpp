
#include "execution/seidel/seidel_serial.hpp"

#include "config.hpp"

namespace ssp4sim::graph
{
    SerialSeidel::SerialSeidel(std::vector<Invocable *> nodes)
        : SeidelBase(nodes),
          log(ssp4cpp::utils::log::make_logger("ssp4sim.execution.SerialSeidel", quill::LogLevel::TraceL1))
    {
        LOG_INFO(log, "[{}] ", __func__);
    }

    /**
     * Traverse the connection graph and invoke nodes when all parents have been invoked for this timestep.
     * [hot path]
     */
    uint64_t SerialSeidel::invoke(StepData step_data)
    {
        IF_LOG({
            LOG_TRACE_L1(log, "[{}] step data: {}", __func__, step_data.to_string());
        });

        int completed = 0;
        reset_counters();

        IF_LOG({
            LOG_TRACE_L1(log, "[{}] Invoking nodes", __func__);
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
                        LOG_TRACE_L2(log, "[{}] Starting {}:{}", __func__, node.id, node.node->name);
                    });

                    node.node->invoke(s);

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
            LOG_TRACE_L1(log, "[{}] End. completed  {}", __func__, completed);
        });

        wait_for_result_collection();

        return step_data.end_time;
    }
}
