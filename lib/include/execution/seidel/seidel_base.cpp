
#include "execution/seidel/seidel_base.hpp"

#include "config.hpp"

#include <utility>

namespace ssp4sim::graph
{

    SeidelBase::SeidelBase(std::vector<Invocable *> _nodes_)
        : ExecutionBase(std::move(_nodes_)),
          log(ssp4cpp::utils::log::make_logger("ssp4sim.execution.SeidelBase")),
          nr_of_nodes(nodes.size()),
          seidel_nodes(nr_of_nodes)
    {
        LOG_INFO(log, "[{func}] ", __func__);
        LOG_DEBUG(log, "[{func}] nr_of_nodes {}, seidel_nodes {}", __func__, nr_of_nodes, seidel_nodes.size());

        for (auto &node : this->nodes)
        {
            auto id = node->id;

            auto &n = seidel_nodes[id];
            n.id = id;
            n.node = node;
            n.nr_parents = node->parents.size();
            n.nr_parents_counter = n.nr_parents;

            LOG_TRACE_L1(log, "[{func}] Assigning SeidelNode {}", __func__, id);
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
