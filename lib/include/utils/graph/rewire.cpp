#include "utils/graph/rewire.hpp"

namespace ssp4sim::utils::graph
{
    void disconnect(Node *a, Node *b)
    {
        a->remove_child(b);
    }

    void redirect(const std::vector<Node *> &scope, Node *from, Node *to)
    {
        for (auto *node : scope)
        {
            node->replace(from, to);
        }
    }

    void condense_component_dag(
        const std::map<std::size_t, std::set<std::size_t>> &dag,
        std::vector<Node *> representatives)
    {
        // Reverse adjacency: predecessor components per component.
        std::map<std::size_t, std::set<std::size_t>> predecessors;
        for (const auto &[src, targets] : dag)
        {
            for (const auto &t : targets)
            {
                predecessors[t].insert(src);
            }
        }

        for (const auto &[i, targets] : dag)
        {
            auto *rep = representatives.at(i);
            // Rebuild the adjacency from the component DAG only: outgoing edges
            // point at successor representatives, incoming at predecessor ones.
            rep->children.clear();
            rep->parents.clear();
            for (const auto &j : targets)
            {
                rep->add_child(representatives.at(j));
            }
            for (const auto &j : predecessors[i])
            {
                rep->add_parent(representatives.at(j));
            }
        }
    }
}