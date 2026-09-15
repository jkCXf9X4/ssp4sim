#pragma once

#include "ssp4sim_definitions.hpp"

#include "executor/executor_base.hpp"

#include "shared_config.hpp"

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
        // from the graph nodes. Every registered factory reads only the typed
        // `ExecutorOptions` this builder was constructed with — neither it nor
        // the executors it constructs touch the global Config.
        using Factory = std::function<std::shared_ptr<ExecutorBase>(
            std::vector<std::shared_ptr<Invocable>>)>;

        ssp4cpp::utils::log::Logger* log = nullptr;

        // `options` is parsed once by `ssp4sim::ExecutorOptions::load()` (see
        // shared_config.hpp). `macro_step` (ns) is the outer macro-step size,
        // owned by `ssp4sim::FmuModelConfig` (SharedConfig::fmu.timestep) — the
        // single source of truth for the configured timestep.
        ExecutorBuilder(const ssp4sim::ExecutorOptions &options,
                        uint64_t macro_step);

        std::string to_string() const override;

        std::shared_ptr<ExecutorBase> build(std::vector<std::shared_ptr<Invocable>> nodes);

    private:
        ssp4sim::ExecutorOptions options_;
        uint64_t macro_step_ = 0;

        // method name -> factory (constructor). Legacy aliases share an entry.
        std::map<std::string, Factory> builders_;
    };

}