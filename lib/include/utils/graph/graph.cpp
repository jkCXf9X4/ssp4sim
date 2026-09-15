#include "utils/graph/graph.hpp"

#include <cstddef>
#include <utility>

namespace ssp4sim::utils::graph
{
    namespace
    {
        bool classify_loop(const std::vector<Node *> &comp)
        {
            return comp.size() > 1
                || (comp.size() == 1
                    && comp[0]->contains_child(comp[0]));
        }

        // Component index -> successor component indices. Intra-component edges
        // are dropped; every cross-component edge becomes a DAG edge. The result
        // is the input format shared by topological_sort() and
        // condense_component_dag().
        std::map<std::size_t, std::set<std::size_t>> build_component_dag(
            const std::vector<std::vector<Node *>> &sccs,
            const std::map<Node *, std::size_t> &node_to_scc)
        {
            std::map<std::size_t, std::set<std::size_t>> dag;
            for (std::size_t i = 0; i < sccs.size(); ++i)
            {
                dag[i] = {};
                for (auto *node : sccs[i])
                {
                    for (auto *child : node->children)
                    {
                        auto child_scc = node_to_scc.at(child);
                        if (child_scc != i)
                        {
                            dag[i].insert(child_scc);
                        }
                    }
                }
            }
            return dag;
        }
    } // anonymous namespace

    Graph::Graph(std::vector<Node *> nodes)
        : nodes(std::move(nodes))
    {
    }

    void Graph::analyze()
    {
        // 1. SCC detection.
        sccs_ = strongly_connected_components(nodes);

        // 2. Node -> SCC index map.
        node_to_scc.clear();
        for (std::size_t i = 0; i < sccs_.size(); ++i)
        {
            for (auto *node : sccs_[i])
            {
                node_to_scc[node] = i;
            }
        }

        // 3. Per-SCC loop classification, then the component DAG + topo order.
        is_loop_.clear();
        is_loop_.reserve(sccs_.size());
        for (const auto &comp : sccs_)
        {
            is_loop_.push_back(classify_loop(comp));
        }
        component_dag_ = build_component_dag(sccs_, node_to_scc);
        execution_order_ = topological_sort(component_dag_);
    }

    const std::vector<std::vector<Node *>> &Graph::sccs() const
    {
        return sccs_;
    }

    const std::vector<bool> &Graph::is_loop() const
    {
        return is_loop_;
    }

    const std::vector<std::size_t> &Graph::execution_order() const
    {
        return execution_order_;
    }

    std::size_t Graph::scc_index_of(Node *node) const
    {
        return node_to_scc.at(node);
    }

    const std::map<std::size_t, std::set<std::size_t>> &Graph::component_dag() const
    {
        return component_dag_;
    }

    bool Graph::verify_placement() const
    {
        // Rank of each SCC in execution_order; -1 if not present.
        std::vector<std::ptrdiff_t> rank(sccs_.size(), -1);
        for (std::size_t pos = 0; pos < execution_order_.size(); ++pos)
        {
            rank[execution_order_[pos]] = static_cast<std::ptrdiff_t>(pos);
        }

        for (std::size_t i = 0; i < sccs_.size(); ++i)
        {
            for (auto *node : sccs_[i])
            {
                for (auto *child : node->children)
                {
                    auto child_scc = node_to_scc.at(child);
                    if (child_scc != i && rank[i] > rank[child_scc])
                    {
                        return false;
                    }
                }
                for (auto *parent : node->parents)
                {
                    auto parent_scc = node_to_scc.at(parent);
                    if (parent_scc != i && rank[i] < rank[parent_scc])
                    {
                        return false;
                    }
                }
            }
        }
        return true;
    }

}