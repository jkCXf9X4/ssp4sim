#include "executor/seidel/seidel_base.hpp"

#include "config.hpp"

#include <cstddef>

namespace ssp4sim::graph
{

    SeidelBase::SeidelBase(std::vector<std::shared_ptr<Invocable>> _nodes_)
        : ExecutorBase(_nodes_, "ssp4sim.execution.SeidelBase"),
          nr_of_nodes(nodes.size()),
          seidel_nodes(nr_of_nodes)
    {
        // Seidel (Gauss-Seidel) runs upstream-to-downstream within a step; a
        // consumer reads its producers' current-step output (end time).
        set_resolver(std::make_shared<ssp4sim::scheduling::DataAccessResolver>(raw_nodes(),
                                                                               ssp4sim::scheduling::AccessMode::EndTime));
        LOG_INFO(log, "[{func}] ", __func__);
        LOG_DEBUG(log, "[{func}] nr_of_nodes {nr_of_nodes}, seidel_nodes {seidel_nodes}", __func__, nr_of_nodes, seidel_nodes.size());

        // Position-based node array: children are indexed 0..n-1, not by their
        // global Node id (component executors carry ids far beyond the model
        // ids). The id -> index vector (sized to Node::id_count()) backs the
        // traversal's child cascade with O(1) lookup.
        index_of_id.assign(ssp4sim::utils::graph::Node::id_count(), npos);
        for (std::size_t i = 0; i < this->nodes.size(); ++i)
        {
            const auto &node = this->nodes[i];

            index_of_id[node->id] = i;
            auto &n = seidel_nodes[i];
            n.id = static_cast<int>(node->id);
            n.node = node.get();
            n.nr_parents = node->parents.size();
            n.nr_parents_counter = n.nr_parents;

            LOG_TRACE_L1(log, "[{func}] Assigning SeidelNode {}", __func__, i);
        }

        LOG_INFO(log, "[{func}] Evaluating start nodes", __func__);
        for (auto &node : this->seidel_nodes)
        {
            if (node.node->nr_parents() == 0)
            {
                start_nodes.push_back(&node);
            }
        }

        // break algebraic loops here?
        // Add a delay of a minor time amount to make sure that the broken loop cant use data that is to new
    }

    void SeidelBase::reset_counters()
    {
        for (auto &n : seidel_nodes)
        {
            n.nr_parents_counter = n.nr_parents;
            n.invoked = false;
        }
    }
}