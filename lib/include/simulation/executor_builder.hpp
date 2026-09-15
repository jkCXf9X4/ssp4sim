#pragma once

#include "ssp4sim_definitions.hpp"

#include "executor/executor_base.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace ssp4sim::graph 
{

    class ExecutorBuilder : public types::IWritable
    {
    public:
        // A strategy: builds the specialized executor for one executor method
        // from the graph nodes. Registration (constructor) and invocation
        // (build) are the only places that touch the global Config for
        // executor selection; the factories read their family's tuning keys
        // and hand the resolved values to the executors' constructors.
        using Factory = std::function<std::shared_ptr<ExecutorBase>(
            std::vector<std::shared_ptr<Invocable>>)>;

        ssp4cpp::utils::log::Logger* log = nullptr;

        ExecutorBuilder();

        std::string to_string() const override;

        std::shared_ptr<ExecutorBase> build(std::vector<std::shared_ptr<Invocable>> nodes);

    private:
        // method name -> factory. Legacy aliases share an entry.
        std::map<std::string, Factory> builders_;
    };

}