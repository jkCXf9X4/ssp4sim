#include "executor/loop_aware/la2_scheduler.hpp"

#include <sstream>
#include <utility>

namespace ssp4sim::graph
{

    // =========================================================================
    //  Construction
    // =========================================================================

    La2Scheduler::La2Scheduler(std::vector<std::shared_ptr<Invocable>> nodes,
                               std::shared_ptr<ExecutorBase> outer,
                               std::shared_ptr<DataAccessResolver> resolver)
        : ExecutorBase(std::move(nodes), "ssp4sim.execution.La2Scheduler"),
          outer(std::move(outer))
    {
        this->name = "La2Scheduler";

        // The scheduler owns the stack's read policy: one flat resolver over
        // all models, installed last so it overwrites any default resolver an
        // inner executor set on direct FmuModel children (intra-SCC edges
        // sample at sub-step start, cross-SCC edges read the latest committed
        // value).
        set_resolver(std::move(resolver));

        LOG_INFO(log, "[{func}] La2Scheduler: outer {outer}",
                 __func__, this->outer->name);
    }

    std::string La2Scheduler::to_string() const
    {
        std::ostringstream oss;
        oss << "La2Scheduler (outer " << outer->name << ", "
            << outer->nodes.size() << " components):\n";
        return oss.str();
    }

    // =========================================================================
    //  Execution - Gauss-Seidel walk over the condensed component graph
    // =========================================================================

    uint64_t La2Scheduler::invoke(StepData step_data)
    {
        return outer->invoke(step_data);
    }

}