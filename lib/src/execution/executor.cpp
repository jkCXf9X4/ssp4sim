#include "execution/executor.hpp"

#include "utils/data_recorder.hpp"

#include "config.hpp"


namespace ssp4sim::graph
{

    ExecutionBase::ExecutionBase(std::vector<Invocable *> nodes) : nodes(std::move(nodes))
    {
        log(trace)("[{}] Setting up shared state", __func__);

        for (std::size_t i = 0; i < this->nodes.size(); i++)
        {
            this->nodes[i]->id = static_cast<uint64_t>(i);
        }
        wait_for_recorder = utils::Config::getOr("simulation.recording.wait_for", false);
    }

    void ExecutionBase::set_recorder(utils::DataRecorder *dr)
    {
        recorder = dr;
    }

    void ExecutionBase::wait_for_result_collection()
    {
        if (wait_for_recorder && recorder)
        {
            recorder->wait_until_done();
        }
    }

    void ExecutionBase::init()
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

}
