#pragma once


#include "ssp4sim_definitions.hpp"

#include "executor.hpp"
#include "invocable.hpp"

#include "cutecpp/log.hpp"


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

        void print(std::ostream &os) const override
        {
            os << "JacobiBase:\n{}\n";
        }

    };
}
