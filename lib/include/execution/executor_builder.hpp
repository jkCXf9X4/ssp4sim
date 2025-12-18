#pragma once

#include "ssp4sim_definitions.hpp"

#include "execution/executor.hpp"

#include "cutecpp/log.hpp"

#include <memory>
#include <vector>

namespace ssp4sim::graph 
{

    class ExecutorBuilder : public types::IPrintable
    {
    public:
        Logger log = Logger("ssp4sim.execution.Delay", LogLevel::info);

        ExecutorBuilder() = default;

        void print(std::ostream &os) const override;

        std::unique_ptr<ExecutionBase> build(std::vector<Invocable *> nodes);
    };

}
