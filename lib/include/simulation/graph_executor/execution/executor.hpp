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

        // the executor should not own the nodes
        std::vector<Invocable *> nodes;

        uint64_t sub_step = 0;

        ExecutionBase(std::vector<Invocable *> nodes);

        void init() override;

        std::string to_string() const
        {
            return this->name + ":\n{}\n";
        }
    };
}
