#pragma once

#include "invocable.hpp"

#include <algorithm>
#include <exception>
#include <execution>
#include <mutex>

namespace ssp4sim::graph
{

    // Run every node in `nodes` once over `step` in parallel, completing the
    // sweep before returning. The first exception thrown by any node is captured
    // and rethrown after the sweep, so a failing member can't trash the group's
    // remaining work with an unwinding mid-parallel loop.
    template <typename NodeList>
    void invoke_group_parallel(const NodeList &nodes, const StepData &step)
    {
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

        if (captured_exception)
        {
            std::rethrow_exception(captured_exception);
        }
    }
}
