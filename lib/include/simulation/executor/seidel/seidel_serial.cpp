#include "executor/seidel/seidel_serial.hpp"

#include "config.hpp"

#include <stdexcept>

namespace ssp4sim::graph
{
    SerialSeidel::SerialSeidel(std::vector<std::shared_ptr<Invocable>> nodes)
        : SeidelBase(nodes)
    {
        LOG_INFO(log, "[{func}] ", __func__);
    }

    /**
     * Traverse the connection graph and invoke nodes when all parents have been invoked for this timestep.
     * [hot path]
     */
    uint64_t SerialSeidel::invoke(StepData step_data)
    {
        IF_LOG({
            LOG_TRACE_L1(log, "[{func}] step data: {}", __func__, step_data.to_string());
        });

        int completed = 0;
        reset_counters();

        IF_LOG({
            LOG_TRACE_L1(log, "[{func}] Invoking nodes", __func__);
        });

        while (nr_of_nodes != completed)
        {
            for (auto &node : seidel_nodes)
            {
                if (node.nr_parents_counter == 0 && !node.invoked)
                {

                    IF_LOG({
                        LOG_TRACE_L2(log, "[{func}] Starting {}:{}", __func__, node.id, node.node->name);
                    });

                    node.node->invoke(step_data);

                    node.invoked = true;
                    completed++;
                    for (auto c : node.node->children)
                    {
                        // O(1) id-indexed lookup into this executor's node set.
                        const std::size_t child_idx = index_of_id[static_cast<Invocable *>(c)->id];
                        if (child_idx == npos)
                        {
                            throw std::runtime_error(
                                "SerialSeidel: child '"
                                + static_cast<Invocable *>(c)->name
                                + "' is not part of this executor's node set "
                                  "(condensed graph edge points outside the "
                                  "component representatives)");
                        }
                        auto &child = seidel_nodes[child_idx];
                        child.nr_parents_counter -= 1;
                    }
                }
            }
        }

        IF_LOG({
            LOG_TRACE_L1(log, "[{func}] End. completed  {}", __func__, completed);
        });

        return step_data.end_time;
    }
}