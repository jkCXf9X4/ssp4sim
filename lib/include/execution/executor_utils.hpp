#pragma once

// #include "cutecpp/log.hpp"

#include "invocable.hpp"

namespace ssp4sim::graph
{

    // continuous input will allow the substep to sample new data during the substep
    inline void invoke_sub_step(Invocable *node, const StepData &step_data, bool continuous_input = false)
    {
        while (node->current_time < step_data.end_time)
        {
            auto substep_start = node->current_time;
            auto substep_end = node->current_time + step_data.timestep;

            auto output_time = substep_end;
            if (node->delay == 0)
            {
                // No delay specified, just set it at the end
                output_time = substep_end;
            }
            else if (substep_start + node->delay <= substep_end)
            {
                // if the step is shorter than the model delay, do the best of it and set it to sub_step_end
                // evaluate if this is true, it could be set to the correct time but there is the potential
                // that the data could be used non-deterministic if a time before substep_end is set...
                output_time = substep_end;
            }
            else if (substep_start + node->delay > substep_end)
            {
                // if the step is longer than the model delay, set the correct time
                output_time = substep_start + node->delay;
            }

            auto s = StepData(substep_start,                                           // start
                              substep_end,                                             // end
                              step_data.timestep,                                      // step_size
                              continuous_input ? substep_start : step_data.start_time, // input
                              output_time);                                            // output_time
            IF_LOG({
                log(info)("Node {}, current {} accumulated_delay: {}, step: {}",
                          node->name, node->current_time, s.to_string());
            });

            node->invoke(s);
        }
    }

}
