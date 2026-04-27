#include "execution/executor.hpp"

#include "config.hpp"

#include <cstddef>
#include <utility>

namespace ssp4sim::graph
{

    ExecutionBase::ExecutionBase(std::vector<Invocable *> nodes)
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.execution.ExecutionBase")),
          nodes(std::move(nodes))
    {
        LOG_TRACE_L1(log, "[{func}] Setting up shared state", __func__);

        for (std::size_t i = 0; i < this->nodes.size(); i++)
        {
            this->nodes[i]->id = static_cast<uint64_t>(i);
        }

        auto recording_enabled = utils::Config::getOr("simulation.recording.wait_for", false);
        wait_for_recorder = recording_enabled && utils::Config::getOr("simulation.recording.wait_for", false);
        sub_step = utils::time::s_to_ns(utils::Config::getOr("simulation.executor.sub_step", utils::Config::getDouble("simulation.timestep")));
    }

    void ExecutionBase::set_recorder(signal::DataRecorder *dr)
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

        LOG_WARNING_LIMIT_EVERY_N(100000, log, "[{func}] TODO: Implement direct feedthrough", __func__);

        // direct feedthrough evaluation should come between these.
        // Doing direct feedthrough for all variables will overwrite inputs with outputs that are unset
        // It should only be done for the relevant algebraic loops. Nothing else!

        for (auto &model : this->nodes)
        {
            model->exit_init();
        }
    }

}
