#include "execution/jacobi/jacobi_parallel_tbb.hpp"

#include "executor_utils.hpp"

#include <execution>
#include <exception>
#include <future>
#include <mutex>
#include <utility>

namespace ssp4sim::graph
{
    JacobiParallelTBB::JacobiParallelTBB(std::vector<Invocable *> nodes) : ExecutionBase(nodes)
    {
        log(info)("[{}] JacobiParallelTBB", __func__);
    }

    uint64_t JacobiParallelTBB::invoke(StepData step_data)
    {
        IF_LOG({
            log(debug)("[{}] stepdata: {}", __func__, step_data.to_string());
        });

        auto step = StepData(step_data.start_time, step_data.end_time, step_data.timestep);
        step.timestep = sub_step;

        std::exception_ptr captured_exception;
        std::mutex exception_mutex;

        std::for_each(std::execution::par, nodes.begin(), nodes.end(),
                      [&](auto &node)
                      {
                        try
                        {
                            node->invoke(step);
                        }
                        catch (...)
                        {
                            std::scoped_lock lock(exception_mutex);
                            if (!captured_exception)
                            {
                                captured_exception = std::current_exception();
                            }
                        }
                      });

        wait_for_result_collection();

        if (captured_exception)
        {
            std::rethrow_exception(captured_exception);
        }

        return step_data.end_time;
    }
}
