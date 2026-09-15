#pragma once

#include "ssp4cpp/utils/log.hpp"

#include "invocable.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ssp4sim::graph
{
    class ExecutorBase : public Invocable
    {
    public:
        ssp4cpp::utils::log::Logger *log = nullptr;

        // shared ownership: executors may wrap each other and the graph may
        // outlive a single executor without move gymnastics
        std::vector<std::shared_ptr<Invocable>> nodes = {};

        // Read policy intentionally lives OUTSIDE the executor: the assembler
        // (ExecutorBuilder's factories / make_la2_stack) derives the resolver
        // and installs it on the models via install_resolver() /
        // install_flat_resolver() (resolver/data_access_resolver.hpp).
        // Constructors are resolver-free.

        ExecutorBase() = default;

        ExecutorBase(std::shared_ptr<Invocable> node,
                     std::string log_name = "ssp4sim.execution.ExecutorBase");

        ExecutorBase(std::vector<std::shared_ptr<Invocable>> nodes,
                     std::string log_name = "ssp4sim.execution.ExecutorBase");

        void init() override;

    protected:
        bool single_node = false;

        std::string to_string() const
        {
            return "ExecutorBase: " + this->name + ":\n{}\n";
        }
    };
}
