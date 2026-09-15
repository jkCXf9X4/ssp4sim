#pragma once

#include "utils/graph/tarjan.hpp"
#include "utils/graph/topological_sort.hpp"
#include "utils/primitives/node.hpp"

#include <cstddef>
#include <map>
#include <set>
#include <vector>

namespace ssp4sim::utils::graph
{
    /// @brief Common graph-analysis interface built on utils::graph::Node.
    ///
    /// A thin read-only audit facade (analyze() + accessors) composed from the
    /// reusable building blocks:
    ///   1. strongly_connected_components() (utils/graph/tarjan.hpp)
    ///   2. loop classification + component DAG build (internal pipeline)
    ///   3. topological_sort()            (utils/graph/topological_sort.hpp)
    ///
    /// All results are a *snapshot* of the node adjacency at analyze() time.
    /// Graph composition changes — e.g. condensation or redirect() — belong to
    /// utils/graph/rewire.hpp and mutate the node adjacency in place; re-run
    /// analyze() afterwards to obtain a fresh snapshot.
    ///
    /// The analysis owns no node memory and never mutates node adjacency.
    ///
    /// Usage:
    ///   Graph graph(nodes);
    ///   graph.analyze();
    ///   // Use sccs(), execution_order(), is_loop(), component_dag()...
    class Graph final
    {
    public:
        // ---- Construction & analysis ----------------------------------------

        explicit Graph(std::vector<Node *> nodes);

        // Run the full analysis pipeline: SCC detection, loop classification,
        // component DAG build and topological sort.
        void analyze();

        // ---- Result accessors (snapshot from analyze(); require analyze() to
        //      have run) -------------------------------------------------------

        // Strongly connected components, each a vector of nodes.
        const std::vector<std::vector<Node *>> &sccs() const;

        // Index-aligned with sccs(): whether each SCC is a feedback loop.
        const std::vector<bool> &is_loop() const;

        // Topological order of the component DAG as indices into sccs().
        const std::vector<std::size_t> &execution_order() const;

        // SCC index of a node.
        std::size_t scc_index_of(Node *node) const;

        // Component DAG: SCC index -> successor SCC indices.
        const std::map<std::size_t, std::set<std::size_t>> &component_dag() const;

        // Verify parent/child placement against the snapshot topological order:
        // valid when every edge points forward, i.e. for every child c of node,
        // order[scc(node)] <= order[scc(c)], and symmetrically for parents.
        // Reads the *current* adjacency against the stored ranking, so analyze()
        // a freshly populated or re-analyzed graph before verifying.
        bool verify_placement() const;

    private:
        std::vector<Node *> nodes;
        std::vector<std::vector<Node *>> sccs_;
        std::vector<bool> is_loop_;
        std::vector<std::size_t> execution_order_;
        std::map<Node *, std::size_t> node_to_scc;
        std::map<std::size_t, std::set<std::size_t>> component_dag_;
    };
}