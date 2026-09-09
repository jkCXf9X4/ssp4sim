#include "graph_analysis/scc_group.hpp"

#include <sstream>

namespace ssp4sim::graph
{

    SccGroup::SccGroup(std::vector<Invocable *> sub_nodes)
        : sub_nodes(std::move(sub_nodes))
    {
        this->name = "SccGroup";
    }

    bool SccGroup::is_loop() const
    {
        // A multi-node SCC is always a loop. A single-node SCC is also a loop
        // when the node has a self-edge (e.g. a feedback self-reference):
        // Tarjan reports such a node as a size-1 SCC, so the size alone is
        // not enough to distinguish a loop from a plain acyclic node.
        return sub_nodes.size() > 1
            || (sub_nodes.size() == 1
                && sub_nodes[0]->contains_child(sub_nodes[0]));
    }

    std::string SccGroup::to_string() const
    {
        std::ostringstream oss;
        oss << "SccGroup:\n";
        for (auto *node : sub_nodes)
        {
            oss << "  - " << node->name << "\n";
        }
        return oss.str();
    }

    uint64_t SccGroup::invoke(StepData step_data)
    {
        for (auto *node : sub_nodes)
        {
            node->invoke(step_data);
        }
        return step_data.end_time;
    }

}