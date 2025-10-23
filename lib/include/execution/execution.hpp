#pragma once

#include "cutecpp/log.hpp"

#include "invocable.hpp"

namespace ssp4sim::graph
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

        void init() override
        {
            for (auto &model : this->nodes)
            {
                model->enter_init();
            }

            log(warning)("[{}] TODO: Implement direct feedthrough", __func__);

        // direct feedthrough evaluation should come between these.
        // Doing direct feedthrough for all variables will overwrite inputs with outputs that are unset
        // It can ony be done for the relevant algebraic loops. Nothing else!
        
            for (auto &model : this->nodes)
            {
                model->exit_init();
            }
        }
    };
}
