#include "graph_analysis/graph_analysis.hpp"

#include "utils/graph/tarjan.hpp"

#include <algorithm>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace ssp4sim::graph
{

    GraphAnalysis::GraphAnalysis(std::vector<Invocable *> nodes)
        : nodes(std::move(nodes)),
          log(ssp4cpp::utils::log::make_logger("ssp4sim.execution.GraphAnalysis"))
    {
        this->name = "GraphAnalysis";
    }

    void GraphAnalysis::analyze()
    {
        // 1. SCC detection.
        sccs = strongly_connected_components(nodes);

        // 2. Node -> SCC index map.
        node_to_scc.clear();
        for (std::size_t i = 0; i < sccs.size(); ++i)
        {
            for (auto *node : sccs[i])
            {
                node_to_scc[node] = i;
            }
        }

        // 3. SCC DAG + topological sort.
        auto dag = build_scc_dag();
        execution_order = topological_sort(dag);

        // 4. Condensed graph.
        build_condensed_graph();
    }

    std::string GraphAnalysis::to_string() const
    {
        std::ostringstream oss;
        oss << "GraphAnalysis:\n";
        oss << "  SCCs: " << sccs.size() << "\n";
        oss << "  Execution steps: " << execution_order.size() << "\n";
        for (std::size_t i = 0; i < execution_order.size(); ++i)
        {
            auto idx = execution_order[i];
            auto &comp = sccs[idx];
            oss << "  Step " << i << " (SCC #" << idx << ", " << comp.size()
                << " nodes):\n";
            for (auto *node : comp)
            {
                oss << "    - " << node->name << "\n";
            }
        }
        return oss.str();
    }

    uint64_t GraphAnalysis::invoke(StepData step_data)
    {
        for (auto scc_idx : execution_order)
        {
            groups[scc_idx]->invoke(step_data);
        }
        return step_data.end_time;
    }

    std::vector<std::vector<Invocable *>> GraphAnalysis::strongly_connected_components(
        const std::vector<Invocable *> &nodes)
    {
        auto raw = ssp4sim::utils::graph::strongly_connected_components(
            ssp4sim::utils::graph::Node::cast_to_parent_ptrs(nodes));

        std::vector<std::vector<Invocable *>> result;
        result.reserve(raw.size());
        for (auto &comp : raw)
        {
            std::vector<Invocable *> casted;
            casted.reserve(comp.size());
            for (auto *n : comp)
            {
                casted.push_back(static_cast<Invocable *>(n));
            }
            result.push_back(std::move(casted));
        }
        return result;
    }

    std::map<std::size_t, std::set<std::size_t>> GraphAnalysis::build_scc_dag() const
    {
        std::map<std::size_t, std::set<std::size_t>> dag;
        for (std::size_t i = 0; i < sccs.size(); ++i)
        {
            dag[i] = {};
            for (auto *node : sccs[i])
            {
                for (auto *child : node->children)
                {
                    auto child_scc = node_to_scc.at(static_cast<Invocable *>(child));
                    if (child_scc != i)
                    {
                        dag[i].insert(child_scc);
                    }
                }
            }
        }
        return dag;
    }

    std::vector<std::size_t> GraphAnalysis::topological_sort(
        const std::map<std::size_t, std::set<std::size_t>> &dag) const
    {
        std::map<std::size_t, std::size_t> in_degree;
        for (auto &[node, _] : dag)
        {
            in_degree[node] = 0;
        }
        for (auto &[_, targets] : dag)
        {
            for (auto &t : targets)
            {
                in_degree[t]++;
            }
        }

        std::queue<std::size_t> q;
        for (auto &[node, deg] : in_degree)
        {
            if (deg == 0)
            {
                q.push(node);
            }
        }

        std::vector<std::size_t> order;
        while (!q.empty())
        {
            auto node = q.front();
            q.pop();
            order.push_back(node);

            for (auto &t : dag.at(node))
            {
                in_degree[t]--;
                if (in_degree[t] == 0)
                {
                    q.push(t);
                }
            }
        }

        if (order.size() != dag.size())
        {
            LOG_ERROR(log, "[{func}] Cycle detected in SCC DAG! "
                           "Ordered {ordered} of {total} components",
                      __func__, order.size(), dag.size());
            // The condensation of SCCs is a DAG by construction, so a cycle
            // here is a structural error (broken SCC computation or graph
            // mutation). Fail loudly instead of silently executing a partial
            // schedule with dropped components.
            throw std::runtime_error(
                "GraphAnalysis::topological_sort: cycle detected in SCC DAG, "
                "topological order truncated (" +
                std::to_string(order.size()) + " of " +
                std::to_string(dag.size()) + " components ordered)");
        }

        return order;
    }

    void GraphAnalysis::build_condensed_graph()
    {
        groups.clear();
        groups.reserve(sccs.size());
        for (auto &comp : sccs)
        {
            groups.push_back(std::make_unique<SccGroup>(comp));
        }

        // Build node -> SCC index map (already computed in analyze(), but
        // rebuild defensively so this method is usable standalone).
        std::map<Invocable *, std::size_t> local_node_to_scc = node_to_scc;
        if (local_node_to_scc.empty())
        {
            for (std::size_t i = 0; i < sccs.size(); ++i)
            {
                for (auto *node : sccs[i])
                {
                    local_node_to_scc[node] = i;
                }
            }
        }

        // Wire each group's children/parents from cross-component edges,
        // deduplicating so the condensed graph has no parallel edges.
        for (std::size_t i = 0; i < sccs.size(); ++i)
        {
            std::set<std::size_t> child_groups;
            std::set<std::size_t> parent_groups;
            for (auto *node : sccs[i])
            {
                for (auto *child : node->children)
                {
                    auto child_scc = local_node_to_scc.at(static_cast<Invocable *>(child));
                    if (child_scc != i)
                    {
                        child_groups.insert(child_scc);
                    }
                }
                for (auto *parent : node->parents)
                {
                    auto parent_scc = local_node_to_scc.at(static_cast<Invocable *>(parent));
                    if (parent_scc != i)
                    {
                        parent_groups.insert(parent_scc);
                    }
                }
            }
            for (auto child_scc : child_groups)
            {
                groups[i]->children.push_back(groups[child_scc].get());
            }
            for (auto parent_scc : parent_groups)
            {
                groups[i]->parents.push_back(groups[parent_scc].get());
            }
        }
    }

    bool GraphAnalysis::verify_placement() const
    {
        // Rank of each SCC in execution_order; -1 if not present.
        std::vector<std::ptrdiff_t> rank(sccs.size(), -1);
        for (std::size_t pos = 0; pos < execution_order.size(); ++pos)
        {
            rank[execution_order[pos]] = static_cast<std::ptrdiff_t>(pos);
        }

        for (std::size_t i = 0; i < sccs.size(); ++i)
        {
            for (auto *node : sccs[i])
            {
                for (auto *child : node->children)
                {
                    auto child_scc = node_to_scc.at(static_cast<Invocable *>(child));
                    if (child_scc != i && rank[i] > rank[child_scc])
                    {
                        return false;
                    }
                }
                for (auto *parent : node->parents)
                {
                    auto parent_scc = node_to_scc.at(static_cast<Invocable *>(parent));
                    if (parent_scc != i && rank[i] < rank[parent_scc])
                    {
                        return false;
                    }
                }
            }
        }
        return true;
    }

    std::size_t GraphAnalysis::scc_index_of(Invocable *node) const
    {
        return node_to_scc.at(node);
    }

    SccGroup *GraphAnalysis::group_of(Invocable *node) const
    {
        return groups[node_to_scc.at(node)].get();
    }

}