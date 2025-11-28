#pragma once

#include "cutecpp/log.hpp"

#include "ssp4sim_definitions.hpp"

#include "executor.hpp"

#include "invocable.hpp"
#include "task_thread_pool.hpp"
#include "task_thread_pool2.hpp"

#include "config.hpp"

#include <assert.h>
#include <execution>

namespace ssp4sim::graph
{
    struct SeidelNode
    {
        int id;
        bool invoked = false;
        Invocable *node;
        std::size_t nr_parents;
        std::size_t nr_parents_counter;
        SeidelNode() {}
    };

    class SeidelBase : public ExecutionBase
    {
    public:
        Logger log = Logger("ssp4sim.execution.SeidelBase", LogLevel::info);

        const int nr_of_nodes = 0;
        std::vector<SeidelNode> seidel_nodes;
        std::vector<SeidelNode *> start_nodes;

        SeidelBase(std::vector<Invocable *> _nodes_);

        void reset_counters();
    };

    class SerialSeidel final : public SeidelBase
    {
    public:
        Logger log = Logger("ssp4sim.execution.SerialSeidel", LogLevel::info);

        SerialSeidel(std::vector<Invocable *> nodes);

        void print(std::ostream &os) const override
        {
            os << "SerialSeidel:\n{}\n";
        }

        // some idea that this might be more effective than looping over all items
        // Not used at the moment
        void invoke_node(SeidelNode &node, StepData step_data);

        /**
         * Traverse the connection graph and invoke nodes when all parents have been invoked for this timestep.
         * [hot path]
         */
        uint64_t invoke(StepData step_data) override final;
    };

    class ParallelSeidel final : public SeidelBase
    {
    public:
        Logger log = Logger("ssp4sim.execution.ParallelSeidel", LogLevel::info);

        ParallelSeidel(std::vector<Invocable *> nodes);

        virtual void print(std::ostream &os) const
        {
            os << "ParallelSeidel:\n{}\n";
        }

        /**
         * Traverse the connection graph and invoke nodes when all parents have been invoked for this timestep.
         * [hot path]
         */
        uint64_t invoke(StepData step_data) override final;
    };
}
