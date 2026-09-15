#include "config.hpp"
#include "executor_builder.hpp"

#include "executor/custom/custom_executors.hpp"

#include "executor/jacobi/jacobi_parallel_fut.hpp"
#include "executor/jacobi/jacobi_parallel_spin.hpp"
#include "executor/jacobi/jacobi_parallel_tbb.hpp"
#include "executor/jacobi/jacobi_serial.hpp"

#include "executor/seidel/seidel_serial.hpp"
#include "executor/seidel/seidel_parallel.hpp"
#include "executor/loop_aware/la2_builder.hpp"
#include "executor/macro/macro_executor.hpp"
#include "executor/macro/realtime_macro_executor.hpp"

#include "utils/time/time.hpp"

#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace ssp4sim::graph
{

    ExecutorBuilder::ExecutorBuilder()
        : log(ssp4cpp::utils::log::make_logger("ssp4sim.execution.ExecutorBuilder"))
    {
        builders_["jacobi"] = [this](std::vector<std::shared_ptr<Invocable>> nodes)
            -> std::shared_ptr<ExecutorBase>
        {
            if (utils::Config::getOr("simulation.executor.jacobi.parallel", false))
            {
                int workers = utils::Config::getOr("simulation.executor.thread_pool_workers", 5);
                int parallel_method = utils::Config::getOr("simulation.executor.jacobi.method", 1);

                if (parallel_method == 1)
                {
                    LOG_INFO(log, "[builder] Executor: JacobiParallelTBB");
                    return std::make_shared<JacobiParallelTBB>(std::move(nodes));
                }
                else if (parallel_method == 2)
                {
                    LOG_INFO(log, "[builder] Executor: JacobiParallelSpin");
                    return std::make_shared<JacobiParallelSpin>(std::move(nodes), workers);
                }
                else if (parallel_method == 3)
                {
                    LOG_INFO(log, "[builder] Executor: JacobiParallelFutures");
                    return std::make_shared<JacobiParallelFutures>(std::move(nodes), workers);
                }
                else
                {
                    throw std::runtime_error("Unknown parallelization method");
                }
            }
            else
            {
                LOG_INFO(log, "[builder] Executor: JacobiSerial");
                return std::make_shared<JacobiSerial>(std::move(nodes));
            }
        };

        builders_["seidel"] = [this](std::vector<std::shared_ptr<Invocable>> nodes)
            -> std::shared_ptr<ExecutorBase>
        {
            if (utils::Config::getOr("simulation.executor.seidel.parallel", false))
            {
                LOG_INFO(log, "[builder] Executor: ParallelSeidel");
                return std::make_shared<ParallelSeidel>(std::move(nodes));
            }
            else
            {
                LOG_INFO(log, "[builder] Executor: SerialSeidel");
                return std::make_shared<SerialSeidel>(std::move(nodes));
            }
        };

        builders_["custom_delay"] = [this](std::vector<std::shared_ptr<Invocable>> nodes)
            -> std::shared_ptr<ExecutorBase>
        {
            LOG_INFO(log, "[builder] Executor: DelayExecutor");
            return std::make_shared<DelayExecutor>(std::move(nodes));
        };

        builders_["custom_delay_partial"] = [this](std::vector<std::shared_ptr<Invocable>> nodes)
            -> std::shared_ptr<ExecutorBase>
        {
            LOG_INFO(log, "[builder] Executor: DelayExecutorPartial");
            return std::make_shared<DelayExecutorPartial>(std::move(nodes));
        };

        // "loop_aware" is the legacy name (see docs/configuration.md and
        // resources/loop_aware_nested*.json); "la2" is the refactored name.
        // Both dispatch to the same loop-aware stack factory.
        builders_["la2"] = [this](std::vector<std::shared_ptr<Invocable>> nodes)
            -> std::shared_ptr<ExecutorBase>
        {
            La2Options options;
            options.mode = utils::Config::getOr(
                "simulation.executor.la2.mode", std::string("linear"));
            options.iterations = utils::Config::getOr(
                "simulation.executor.la2.iterations", -1);
            options.factor = utils::Config::getOr(
                "simulation.executor.la2.factor", 0.8);
            options.max_steps = static_cast<std::size_t>(utils::Config::getOr(
                "simulation.executor.la2.max_steps", 64));
            options.min_substep_fraction = utils::Config::getOr(
                "simulation.executor.la2.min_substep_fraction", 0.001);
            options.parallel = utils::Config::getOr(
                "simulation.executor.la2.parallel", false);

            LOG_INFO(log, "[builder] Executor: La2");
            return make_la2_stack(std::move(nodes), options);
        };
        builders_["loop_aware"] = builders_["la2"];

        // ParallelSeidel is NOT implemented: its invoke() throws "This is not
        // implemented" at runtime. Fail at dispatch time with a clear,
        // actionable message instead of silently mis-dispatching or falling
        // through to the generic "Unknown executor method".
        builders_["parallel_seidel"] = [](std::vector<std::shared_ptr<Invocable>>)
            -> std::shared_ptr<ExecutorBase>
        {
            throw std::runtime_error(
                "Executor method 'parallel_seidel' requests ParallelSeidel, "
                "which is NOT implemented. "
                "Use 'seidel' (serial) or 'jacobi' instead.");
        };
        builders_["parallel-seidel"] = builders_["parallel_seidel"];
    }

    std::string ExecutorBuilder::to_string() const
    {
        return "ExecutorBuilder:\n{}\n";
    }

    std::shared_ptr<ExecutorBase> ExecutorBuilder::build(std::vector<std::shared_ptr<Invocable>> nodes)
    {
        // Select the strategy from config and build the specialized executor;
        // the same factory call resolves la2's nested stack. Everything is
        // wrapped in a MacroExecutor sized from `simulation.timestep`.

        auto executor_method = utils::Config::getOr("simulation.executor.method", std::string("jacobi"));

        auto factory = builders_.find(executor_method);
        if (factory == builders_.end())
        {
            throw std::runtime_error("Unknown executor method: '" + executor_method + "'");
        }

        auto specialized_executor = factory->second(std::move(nodes));

        const auto macro_step =
            utils::time::s_to_ns(utils::Config::getDouble("simulation.timestep"));

        if (utils::Config::getOr("simulation.realtime", false))
        {
            return std::make_shared<graph::RealtimeMacroExecutor>(
                std::vector<std::shared_ptr<Invocable>>{specialized_executor},
                macro_step);
        }
        else
        {
            return std::make_shared<graph::MacroExecutor>(
                std::vector<std::shared_ptr<Invocable>>{specialized_executor},
                macro_step);
        }
    }

}