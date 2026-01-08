#pragma once

#include "ssp4sim_definitions.hpp"

#include "execution/executor.hpp"

#include "cutecpp/log.hpp"

#include <memory>
#include <string>
#include <vector>

namespace ssp4sim::graph 
{

    class ExecutorBuilder : public types::IWritable
    {
    public:
        Logger log = Logger("ssp4sim.execution.Delay", LogLevel::info);

        ExecutorBuilder() = default;

        std::string to_string() const override;

        std::unique_ptr<ExecutionBase> build(std::vector<Invocable *> nodes);
    };

}
