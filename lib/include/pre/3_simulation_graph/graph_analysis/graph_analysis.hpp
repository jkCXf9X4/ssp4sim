#pragma once

#include "invocable.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace ssp4sim::utils::graph
{
    class Node;
}

namespace ssp4sim::graph
{

    /**
     * @brief Graph analysis: SCC detection, component DAG, and topological sort.
     *
     * This is a pure analysis utility, not an Invocable. It owns no node
     * memory (input nodes are treated as read-only).
     *
     * Usage:
     *   GraphAnalysis analysis(nodes);
     *   analysis.analyze();
     *   // Use sccs, execution_order, is_loop...
     *
     * Algorithm pipeline (analyze):
     *   1. Tarjan's SCC on the full node set.
     *   2. Build a DAG of SCCs (component -> component edges).
     *   3. Topologically sort the component DAG.
     */
    class GraphAnalysis final
    {
    public:
        // ---- Input / Output -------------------------------------------------
        // The original nodes the analysis operates on.
        std::vector<Invocable *> nodes;

        // SCCs computed from the graph, each is a vector of nodes.
        std::vector<std::vector<Invocable *>> sccs;

        // For each SCC index, whether it is a loop: more than one node, or a
        // single node with a self-edge (feedback self-reference).
        std::vector<bool> is_loop;

        // Topologically ordered indices into sccs.
        std::vector<std::size_t> execution_order;

        // ---- Construction & Analysis ----------------------------------------

        explicit GraphAnalysis(std::vector<Invocable *> nodes);

        // Run the full analysis pipeline: SCC detection, DAG build and
        // topological sort.
        void analyze();

        // ---- Result accessors -----------------------------------------------

        // SCC index of a node (requires analyze() to have run).
        std::size_t scc_index_of(Invocable *node) const;

        // Verify parent/child placement in the graph in relation to a node.
        // A placement is valid when every edge always points forward in the
        // topological order, i.e. for every child c of node,
        // order[scc(node)] <= order[scc(c)], and symmetrically for parents.
        bool verify_placement() const;

        // ---- Standalone utilities (no analyze() required) -------------------

        // Find strongly connected components of the given nodes.
        static std::vector<std::vector<Invocable *>> strongly_connected_components(
            const std::vector<Invocable *> &nodes);

        // Kahn's topological sorting of a DAG (map from node to set of targets).
        static std::vector<std::size_t> topological_sort(
            const std::map<std::size_t, std::set<std::size_t>> &dag);

    private:
        // Per-node SCC index (set by analyze).
        std::map<Invocable *, std::size_t> node_to_scc;

        // Build the map from source SCC index to target SCC indices.
        std::map<std::size_t, std::set<std::size_t>> build_scc_dag() const;

        // Internal logger.
        ssp4cpp::utils::log::Logger *log;
    };

}