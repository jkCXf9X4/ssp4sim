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

        std::exception_ptr captured_exception;
        std::mutex exception_mutex;

        std::for_each(std::execution::par, nodes.begin(), nodes.end(),
                      [&](auto &node)
                      {
                          try
                          {
                              node->invoke(step_data);
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

        if (captured_exception)
        {
            std::rethrow_exception(captured_exception);
        }

        return step_data.end_time;
    }
}