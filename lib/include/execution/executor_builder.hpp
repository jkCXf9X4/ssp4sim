#pragma once

#include "cutecpp/log.hpp"

#include "ssp4sim_definitions.hpp"

#include "executor.hpp"
#include "config.hpp"

#include "jacobi.hpp"
#include "seidel.hpp"
#include "custom_executors.hpp"

#include <assert.h>
#include <execution>

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
