#pragma once

#include "utils/log.hpp"

#include "invocable.hpp"

namespace ssp4sim::sim::graph
{

    class ExecutionBase : public Invocable
    {
    public:
        Logger log = Logger("ssp4sim.execution.ExecutionBase", LogLevel::info);
        std::vector<Invocable *> nodes;


        ExecutionBase(std::vector<Invocable *> nodes) : nodes(std::move(nodes))
        {
            log(trace)("[{}] Setting up shared state", __func__);

            for (int i = 0; i < this->nodes.size(); i++)
            {
                this->nodes[i]->id = i;
            }
        }
    };
}
