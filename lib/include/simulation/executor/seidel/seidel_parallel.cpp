#include "executor/seidel/seidel_parallel.hpp"

#include "config.hpp"

#include <stdexcept>

namespace ssp4sim::graph
{
    ParallelSeidel::ParallelSeidel(std::vector<std::shared_ptr<Invocable>> nodes)
        : SeidelBase(nodes)
    {
        LOG_INFO(log, "[{func}]", __func__);
    }

    /**
     * Parallel Seidel traversal is not implemented yet; invoke() always
     * throws std::runtime_error("This is not implemented").
     */
    uint64_t ParallelSeidel::invoke(StepData step_data)
    {
        IF_LOG({
            LOG_TRACE_L1(log, "[{func}] step data: {}", __func__, step_data.to_string());
        });

        throw std::runtime_error("This is not implemented");


        return step_data.end_time;
    }

}