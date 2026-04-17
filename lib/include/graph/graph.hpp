#pragma once

#include "ssp4sim_definitions.hpp"

#include "invocable.hpp"
#include "executor.hpp"

#include "signal/recorder.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <cstdint>
#include <map>
#include <memory>
#include <string>
#include <vector>


namespace ssp4sim::graph
{
    class Graph final : public Invocable
    {
    public:
        quill::Logger* log = nullptr;

        std::map<std::string, Invocable*> node_map;
        std::vector<Invocable *> nodes;

        std::unique_ptr<ExecutionBase> executor;
        ssp4sim::signal::DataRecorder *recorder = nullptr;

        Graph() = default;

        Graph(std::map<std::string, Invocable *> node_map, ssp4sim::signal::DataRecorder *recorder);

        std::string to_string() const override;

        void init();

        uint64_t invoke(StepData step_data) override final;
    };

}
