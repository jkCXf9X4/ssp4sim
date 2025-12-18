#include "execution/jacobi/jacobi_serial.hpp"

#include "executor_utils.hpp"

#include <execution>
#include <exception>
#include <future>
#include <mutex>
#include <utility>

namespace ssp4sim::graph
{

    JacobiSerial::JacobiSerial(std::vector<Invocable *> nodes) : JacobiBase(nodes)
    {
        log(info)("[{}] ", __func__);
    }

    uint64_t JacobiSerial::invoke(StepData step_data)
    {
        auto step = StepData(step_data.start_time, step_data.end_time, sub_step, step_data.start_time, step_data.end_time);

        IF_LOG({
            log(debug)("[{}] stepdata: {}", __func__, step_data.to_string());
        });

        for (auto &node : this->nodes)
        {
            node->invoke(step);
        }

        wait_for_result_collection();

        return step_data.end_time;
    }
}
