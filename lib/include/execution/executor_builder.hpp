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
        quill::Logger* log = nullptr;

        ExecutorBuilder()
            : log(ssp4cpp::utils::log::make_logger("ssp4sim.execution.Delay", quill::LogLevel::TraceL1))
        {
        }

        std::string to_string() const override;

        std::unique_ptr<ExecutionBase> build(std::vector<Invocable *> nodes);
    };

}
