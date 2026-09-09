#include "macro/macro_executor.hpp"
#include "config.hpp"

namespace ssp4sim::graph
{

    MacroExecutor::MacroExecutor(std::vector<Invocable *> nodes)
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.graph.MacroExecutor")), nodes(std::move(nodes))
    {
        macro_step = utils::time::s_to_ns(utils::Config::getDouble("simulation.timestep"));
    }

    uint64_t MacroExecutor::invoke(StepData step_data)
    {
        IF_LOG({
            LOG_DEBUG(log, "[{func}] Invoking Graph, full step: {step}", __func__, step_data.to_string());
        });

        auto t = step_data.start_time;
        while (t < step_data.end_time)
        {

            auto s = StepData(t, t + step_data.timestep);

            IF_LOG({
                LOG_TRACE_L1(log, "[{func}] Graph executing step: {}", __func__, s.to_string());
            });

            for (auto node: nodes)
            {
                node->invoke(s);
            }

            t += step_data.timestep;
        }

        return t;
    }

}
