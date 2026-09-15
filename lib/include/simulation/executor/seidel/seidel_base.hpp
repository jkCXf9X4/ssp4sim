#pragma once


#include "ssp4sim_definitions.hpp"

#include "executor_base.hpp"
#include "invocable.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
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

    class SeidelBase : public ExecutorBase
    {
    public:
        const int nr_of_nodes = 0;
        std::vector<SeidelNode> seidel_nodes;
        std::vector<SeidelNode *> start_nodes;

        SeidelBase(std::vector<std::shared_ptr<Invocable>> _nodes_);

        // Node id -> index into seidel_nodes. A vector indexed by the node's
        // process-wide id (dense 0..Node::id_count()), sized once at
        // construction; ids outside this executor's node set map to npos (an
        // edge pointing outside the component representatives). O(1) lookup on
        // the hot path.
        static constexpr std::size_t npos = std::size_t(-1);
        std::vector<std::size_t> index_of_id;

        void reset_counters();
    };
}