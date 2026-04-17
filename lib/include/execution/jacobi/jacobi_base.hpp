#pragma once


#include "ssp4sim_definitions.hpp"

#include "executor.hpp"
#include "invocable.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <vector>

namespace ssp4sim::graph
{
class JacobiBase : public ExecutionBase
    {
    public:
        quill::Logger* log = nullptr;

        JacobiBase(std::vector<Invocable *> nodes)
            : ExecutionBase(nodes),
              log(ssp4cpp::utils::log::make_logger("ssp4sim.execution.JacobiBase", quill::LogLevel::TraceL1))
        {
            LOG_INFO(log, "[{}] ", __func__);
        }

        std::string to_string() const override
        {
            return "JacobiBase:\n{}\n";
        }

    };
}
