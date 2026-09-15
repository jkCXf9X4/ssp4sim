#include "executor/executor_base.hpp"

#include "config.hpp"

#include <cstddef>

namespace ssp4sim::graph
{


    ExecutorBase::ExecutorBase(std::shared_ptr<Invocable> node, std::string log_name) : log(ssp4cpp::utils::log::make_logger(log_name))
    {
        nodes.push_back(node);
        single_node = nodes.size() == 1;
    }

    ExecutorBase::ExecutorBase(std::vector<std::shared_ptr<Invocable>> nodes, std::string log_name)
        : log(ssp4cpp::utils::log::make_logger(log_name)),
          nodes(nodes)
    {
        single_node = nodes.size() == 1;
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
