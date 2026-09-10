#pragma once


#include "ssp4sim_definitions.hpp"

#include "executor.hpp"
#include "invocable.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <memory>
#include <vector>

namespace ssp4sim::graph
{
class JacobiBase : public ExecutionBase
    {
    public:
        JacobiBase(std::vector<std::shared_ptr<Invocable>> nodes)
            : ExecutionBase(nodes, "ssp4sim.execution.JacobiBase")
        {
            LOG_INFO(log, "[{func}] ", __func__);
        }

        std::string to_string() const override
        {
            return "JacobiBase:\n{}\n";
        }

    };
}