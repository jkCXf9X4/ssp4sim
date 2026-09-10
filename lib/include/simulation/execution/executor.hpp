#pragma once

#include "ssp4cpp/utils/log.hpp"

#include "invocable.hpp"

#include "resolver/data_access_resolver.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace ssp4sim::graph
{
    using DataAccessResolver = ssp4sim::scheduling::DataAccessResolver;

    class ExecutionBase : public Invocable
    {
    public:
        ssp4cpp::utils::log::Logger* log = nullptr;

        // shared ownership: executors may wrap each other and the graph may
        // outlive a single executor without move gymnastics
        std::vector<std::shared_ptr<Invocable>> nodes;

        // data_access_resolver to be added here

        ExecutionBase() = default;

        ExecutionBase(std::vector<std::shared_ptr<Invocable>> nodes, std::string log_name="ssp4sim.execution.ExecutionBase");

        void set_resolver(std::shared_ptr<DataAccessResolver> resolver);

        void init() override;

        std::string to_string() const
        {
            return "ExecutionBase: " + this->name + ":\n{}\n";
        }
    };
}
