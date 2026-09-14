#include "executor/jacobi/jacobi_parallel_tbb.hpp"

#include "executor_utils.hpp"
#include "resolver/data_access_resolver.hpp"

namespace ssp4sim::graph
{
    JacobiParallelTBB::JacobiParallelTBB(std::vector<std::shared_ptr<Invocable>> nodes)
        : ExecutorBase(nodes, "ssp4sim.execution.JacobiParallelTBB")
    {
        set_resolver(std::make_shared<ssp4sim::scheduling::DataAccessResolver>(raw_nodes(),
                                                                               ssp4sim::scheduling::AccessMode::StartTime));
        LOG_INFO(log, "[{func}] JacobiParallelTBB", __func__);
    }

    uint64_t JacobiParallelTBB::invoke(StepData step_data)
    {
        IF_LOG({
            LOG_DEBUG(log, "[{func}] stepdata: {stepdata}", __func__, step_data.to_string());
        });

        auto step = StepData(step_data.start_time, step_data.end_time);

        invoke_group_parallel(nodes, step);

        return step_data.end_time;
    }
}