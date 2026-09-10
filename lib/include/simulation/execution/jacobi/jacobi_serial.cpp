#include "execution/jacobi/jacobi_serial.hpp"

namespace ssp4sim::graph
{

    JacobiSerial::JacobiSerial(std::vector<std::shared_ptr<Invocable>> nodes)
        : JacobiBase(nodes)
    {
        LOG_INFO(log, "[{func}] ", __func__);
    }

    uint64_t JacobiSerial::invoke(StepData step_data)
    {
        auto step = StepData(step_data.start_time, step_data.end_time);

        IF_LOG({
            LOG_DEBUG(log, "[{func}] stepdata: {stepdata}", __func__, step_data.to_string());
        });

        for (auto &node : this->nodes)
        {
            node->invoke(step);
        }

        return step_data.end_time;
    }
}