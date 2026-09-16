#pragma once

#include "utils/primitives/node.hpp"

#include <cstddef>
#include <map>
#include <set>
#include <vector>

namespace ssp4sim::utils::graph
{
    /// @brief Remove the connection between two nodes in both directions.
    ///
    /// No-op when no connection exists, so callers can disconnect safely.
    void disconnect(Node *a, Node *b);

    /// @brief Replace @p from by @p to in every node of @p scope.
    ///
    /// For each node in @p scope, any child or parent pointer equal to @p from
    /// is replaced by @p to (via Node::replace). Duplicate connections are
    /// absorbed by Node's bidirectional guards.
    void redirect(const std::vector<Node *> &scope, Node *from, Node *to);

    /// @brief Condense a component DAG into representative nodes.
    ///
    /// Replaces every component of a graph by its representative node: for each
    /// component index `i`, `representatives[i]` stands in for that whole
    /// component, so every cross-component edge `i -> j` becomes
    /// `representatives[i] -> representatives[j]`. Intra-component edges are
    /// dropped (the representative subsumes its members).
    ///
    /// The representative adjacency (Node::children / Node::parents) is rebuilt
    /// in place; component members other than a representative are NOT touched,
    /// which keeps model-level data edges (e.g. `model->connections`) intact for
    /// resolver lookups that walk them independently.
    ///
    /// @param dag   Component index -> successor component indices. Must contain
    ///              an entry (possibly empty) for every component index.
    /// @param representatives index-aligned with `dag`: the node that replaces
    ///              component `i`. Acyclic members may be their own
    ///              representatives (the node itself); loop components should be
    ///              represented by a dedicated executor node.
    void condense_component_dag(
        const std::map<std::size_t, std::set<std::size_t>> &dag,
        std::vector<Node *> representatives);
}