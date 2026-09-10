#include "execution/executor.hpp"

#include "config.hpp"

#include "pre/3_simulation_graph/elements/model_fmu.hpp"

#include <cstddef>

namespace ssp4sim::graph
{

ExecutionBase::ExecutionBase(std::vector<std::shared_ptr<Invocable>> nodes, std::string log_name)
        : log(ssp4cpp::utils::log::make_logger(log_name)),
          nodes(nodes)
    {

    }

    void ExecutionBase::set_resolver(std::shared_ptr<DataAccessResolver> resolver)
    {
        for (const auto &node : nodes)
        {
            if (auto fmu = dynamic_cast<FmuModel *>(node.get()))
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
