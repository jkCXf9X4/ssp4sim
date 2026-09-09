#pragma once

#include "ssp4sim_definitions.hpp"

#include "invocable.hpp"
#include "executor.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>


namespace ssp4sim::graph
{

    // TODO: This is not an executor, its a graph wrapper 
    // evaluate if it should be renamed or absorbed into something els
// should this be a macro step executor?

    class GraphExecutor final : public Invocable
    {
    public:
        ssp4cpp::utils::log::Logger* log = nullptr;

        std::map<std::string, Invocable*> node_map;
        std::vector<Invocable *> nodes;

        std::unique_ptr<ExecutionBase> executor;

        GraphExecutor() = default;

        GraphExecutor(std::map<std::string, Invocable *> node_map);

        std::string to_string() const override;

        void init();

        uint64_t invoke(StepData step_data) override final;
    };

}
