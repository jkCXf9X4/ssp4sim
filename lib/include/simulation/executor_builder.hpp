#pragma once

#include "ssp4sim_definitions.hpp"

#include "executor/executor_base.hpp"

#include "shared_config.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ssp4sim::graph 
{

    class ExecutorBuilder : public types::IWritable
    {
    public:
        // A factory: builds the specialized executor for one executor variant
        // from the graph nodes. Every registered factory reads only the typed
        // `ExecutorOptions` this builder was constructed with — neither it nor
        // the executors it constructs touch the global Config.
        using Factory = std::function<std::shared_ptr<ExecutorBase>(
            std::vector<std::shared_ptr<Invocable>>)>;

        // A registered leaf variant: the concrete executor (family + mode)
        // that `selects` accepts, and the factory that constructs it from the
        // graph nodes. The config set (`ExecutorOptions`) decides which
        // variant applies; build() resolves it and runs the factory.
        struct Variant
        {
            std::string name;
            std::function<bool(const ssp4sim::ExecutorOptions &)> selects;
            Factory factory;
        };

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

        // Every concrete executor (family + mode) registers as one leaf
        // variant; build() picks the single matching variant from the config
        // set instead of branching on option combinations. Legacy aliases
        // share a selector entry.
        std::vector<Variant> variants_;

        void register_variant(std::string name,
                              std::function<bool(const ssp4sim::ExecutorOptions &)> selects,
                              Factory factory);
    };

}