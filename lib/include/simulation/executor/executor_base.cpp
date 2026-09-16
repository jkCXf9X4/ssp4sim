#include "executor/executor_base.hpp"

#include "config.hpp"
#include "utils/time/time.hpp"

#include <chrono>
#include <cstddef>
#include <thread>

namespace ssp4sim::graph
{


    ExecutorBase::ExecutorBase(std::shared_ptr<Invocable> node,
                               std::string log_name, const bool realtime)
        : log(ssp4cpp::utils::log::make_logger(log_name)),
          realtime(realtime)
    {
        nodes.push_back(node);
        single_node = nodes.size() == 1;
        realtime_start_reference = realtime ? utils::time::time_now_ns() : 0;
    }

    ExecutorBase::ExecutorBase(std::vector<std::shared_ptr<Invocable>> nodes,
                               std::string log_name, const bool realtime)
        : log(ssp4cpp::utils::log::make_logger(log_name)),
          realtime(realtime),
          nodes(nodes)
    {
        single_node = nodes.size() == 1;
        realtime_start_reference = realtime ? utils::time::time_now_ns() : 0;
    }

    void ExecutorBase::wait_for_realtime_sync(uint64_t simulation_time)
    {
        if (!realtime)
        {
            return;
        }

        using clock = std::chrono::steady_clock;
        auto target = clock::time_point(
            std::chrono::nanoseconds(realtime_start_reference + simulation_time));
        std::this_thread::sleep_until(target);

        IF_LOG({
            LOG_INFO(log, "[{func}] Realtime: {time}, TargetTime: {target}",
                     __func__, simulation_time, target.time_since_epoch().count());
        });
    }

    void ExecutorBase::init()
    {
        // Recursive init: this executor's children may themselves be executors
        // (e.g. MacroExecutor -> Jacobi/TBB -> FmuModel), so dispatch through the
        // virtual Invocable::init() to reach the leaf models (enter/exit init).
        for (auto &model : this->nodes)
        {
            model->init();
        }
    }
}
