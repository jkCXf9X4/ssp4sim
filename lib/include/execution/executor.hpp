#pragma once

#include "cutecpp/log.hpp"

#include "invocable.hpp"
#include "config.hpp"

namespace ssp4sim::graph
{

    class ExecutionBase : public Invocable
    {
    public:
        Logger log = Logger("ssp4sim.execution.ExecutionBase", LogLevel::info);
        // the executor should not own the nodes
        std::vector<Invocable *> nodes;

        utils::DataRecorder *recorder = nullptr;
        const bool wait_for_recorder = utils::Config::getOr<bool>("simulation.recording.wait_for", false);

        ExecutionBase(std::vector<Invocable *> nodes) : nodes(nodes)
        {
            log(trace)("[{}] Setting up shared state", __func__);

            
            for (int i = 0; i < this->nodes.size(); i++)
            {
                this->nodes[i]->id = i;
            }
        }
        
        void set_recorder(utils::DataRecorder *dr)
        {
            recorder = dr;
        }

        inline void wait_for_result_collection()
        {
            if (wait_for_recorder && recorder)
            {
                recorder->wait_until_done();
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
            // It should only be done for the relevant algebraic loops. Nothing else!

            for (auto &model : this->nodes)
            {
                model->exit_init();
            }
        }
    };
}
