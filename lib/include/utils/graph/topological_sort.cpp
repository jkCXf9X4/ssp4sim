#include "utils/graph/topological_sort.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <queue>
#include <stdexcept>
#include <string>

namespace ssp4sim::utils::graph
{
    std::vector<std::size_t> topological_sort(
        const std::map<std::size_t, std::set<std::size_t>> &dag)
    {
        auto *log = ssp4cpp::utils::log::make_logger("ssp4sim.graph.topological_sort");

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
            throw std::runtime_error(
                "utils::graph::topological_sort: cycle detected in component "
                "DAG, topological order truncated (" +
                std::to_string(order.size()) + " of " +
                std::to_string(dag.size()) + " components ordered)");
        }

        return order;
    }
}