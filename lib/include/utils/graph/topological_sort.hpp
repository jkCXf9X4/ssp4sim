#pragma once

#include <cstddef>
#include <map>
#include <set>
#include <vector>

namespace ssp4sim::utils::graph
{
    /// @brief Kahn's algorithm topological ordering of a DAG.
    ///
    /// The DAG is represented as `node index -> successor node indices`, the
    /// same representation used by condense_component_dag()
    /// (utils/graph/rewire.hpp). Must contain an entry (possibly empty) for
    /// every node index.
    ///
    /// @param dag  Node index -> successor node indices.
    /// @return Node indices in topological order.
    /// @throws std::runtime_error if the DAG contains a cycle (the condensed
    ///         component graph is a DAG by construction, so a cycle is a
    ///         structural error).
    std::vector<std::size_t> topological_sort(
        const std::map<std::size_t, std::set<std::size_t>> &dag);
}