#pragma once

#include "ssp4sim_definitions.hpp"

#include "executor/executor_base.hpp"

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

        std::shared_ptr<ExecutorBase> build(std::vector<std::shared_ptr<Invocable>> nodes);

    private:
        // Assembles the la2 stack (SCC detection, per-loop SubstepExecutors,
        // condensed component DAG, outer Gauss-Seidel executor and the
        // stack-wide read-path resolver) from config. All la2 parameter reads
        // are confined here so the executors stay Config-free.
        std::shared_ptr<ExecutorBase> build_la2(std::vector<std::shared_ptr<Invocable>> nodes);
    };

}
