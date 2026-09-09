#pragma once

#include "ssp4cpp/utils/log.hpp"

#include "invocable.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace ssp4sim::graph
{
    class ExecutionBase : public Invocable
    {
    public:
        ssp4cpp::utils::log::Logger* log = nullptr;

        std::vector<std::unique_ptr<Invocable>> nodes;

        // data_access_resolver to be added here

        ExecutionBase() = default;

        ExecutionBase(std::vector<std::unique_ptr<Invocable>> nodes);

        void set_resolver(std::shared_ptr<DataAccessResolver> &resolver);

        void init() override;

        std::string to_string() const
        {
            return "ExecutionBase: " + this->name + ":\n{}\n";
        }
    };
}
