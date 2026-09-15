#include "executor/executor_base.hpp"

#include "config.hpp"

#include "pre/3_simulation_graph/elements/model_fmu.hpp"

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

    void ExecutorBase::set_resolver(std::shared_ptr<DataAccessResolver> resolver)
    {
        for (const auto &node : nodes)
        {
            if (auto fmu = dynamic_cast<FmuModel *>(node.get()))
            {
                fmu->access_resolver = resolver;
            }
        }
    }

    std::vector<Invocable *> ExecutorBase::raw_nodes() const
    {
        std::vector<Invocable *> raw;
        raw.reserve(nodes.size());
        for (const auto &node : nodes)
        {
            raw.push_back(node.get());
        }
        return raw;
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
