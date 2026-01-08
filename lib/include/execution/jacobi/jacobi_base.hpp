#pragma once


#include "ssp4sim_definitions.hpp"

#include "executor.hpp"
#include "invocable.hpp"

#include "cutecpp/log.hpp"

#include <vector>

namespace ssp4sim::graph
{
class JacobiBase : public ExecutionBase
    {
    public:
        Logger log = Logger("ssp4sim.execution.JacobiBase", LogLevel::info);

        JacobiBase(std::vector<Invocable *> nodes) : ExecutionBase(nodes)
        {
            log(info)("[{}] ", __func__);
        }

        std::string to_string() const override
        {
            return "JacobiBase:\n{}\n";
        }

    };
}
