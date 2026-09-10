#pragma once

#include "ssp4sim_definitions.hpp"

#include "execution/executor.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <memory>
#include <string>
#include <vector>

namespace ssp4sim::graph 
{

    class ExecutorBuilder : public types::IWritable
    {
    public:
        ssp4cpp::utils::log::Logger* log = nullptr;

        ExecutorBuilder()
            : log(ssp4cpp::utils::log::make_logger("ssp4sim.execution.ExecutorBuilder"))
        {
        }

        std::string to_string() const override;

        std::shared_ptr<ExecutionBase> build(std::vector<std::shared_ptr<Invocable>> nodes);
    };

}
