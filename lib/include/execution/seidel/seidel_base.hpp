#pragma once


#include "ssp4sim_definitions.hpp"

#include "executor.hpp"
#include "invocable.hpp"

#include "cutecpp/log.hpp"

#include <cstddef>
#include <vector>

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
}
