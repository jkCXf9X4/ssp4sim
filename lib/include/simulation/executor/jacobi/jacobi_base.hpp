#pragma once


#include "ssp4sim_definitions.hpp"

#include "executor_base.hpp"
#include "invocable.hpp"

#include "resolver/data_access_resolver.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <memory>
#include <vector>

namespace ssp4sim::graph
{
class JacobiBase : public ExecutorBase
    {
    public:
        JacobiBase(std::vector<std::shared_ptr<Invocable>> nodes)
            : ExecutorBase(nodes, "ssp4sim.execution.JacobiBase")
        {
            // Jacobi samples every producer at the sub-step start.
            set_resolver(std::make_shared<ssp4sim::scheduling::DataAccessResolver>(raw_nodes(),
                                                                                   ssp4sim::scheduling::AccessMode::StartTime));
            LOG_INFO(log, "[{func}] ", __func__);
        }

        std::string to_string() const override
        {
            return "JacobiBase:\n{}\n";
        }

    };
}