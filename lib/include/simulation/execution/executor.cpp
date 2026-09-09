#include "execution/executor.hpp"

#include "config.hpp"

#include <cstddef>
#include <utility>

namespace ssp4sim::graph
{

    ExecutionBase::ExecutionBase(std::vector<std::unique_ptr<Invocable>> nodes)
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.execution.ExecutionBase")),
          nodes(std::move(nodes))
    {

    }

    void ExecutionBase::set_resolver(std::shared_ptr<DataAccessResolver> &resolver)
    {
        for (const auto &node : nodes)
        {
            if (auto fmu = dynamic_cast<std::unique_ptr<graph::FmuModel>>(node))
            {
                fmu->access_resolver = resolver;
            }
        }
    }

    void ExecutionBase::init()
    {
        for (auto &model : this->nodes)
        {
            model->enter_init();
        }

        // Do NOT implement direct feedthrough for CO-SImulation, see https://github.com/modelica/fmi-standard/discussions/2066

        for (auto &model : this->nodes)
        {
            model->exit_init();
        }
    }
}
