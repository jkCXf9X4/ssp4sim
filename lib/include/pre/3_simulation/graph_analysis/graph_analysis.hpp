#pragma once

#include "invocable.hpp"

#include "graph_analysis/scc_group.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <cstddef>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace ssp4sim::graph
{

    /**
     * @brief Reusable graph analysis and manipulation for schedulers.
     *
     * Owns no node memory; it treats the input node graph as read-only (the
     * SccGroup wrappers produced by build_condensed_graph are owned here and
     * destroyed with this object).
     *
     * Analysis phase (analyze):
     * 1. Run Tarjan's SCC on the full node set.
     * 2. Build a DAG of SCCs (component -> component edges).
     * 3. Topologically sort the component DAG.
     * 4. Build the equivalent condensed graph: every SCC is wrapped in an
     *    SccGroup Invocable whose children/parents form the SCC DAG.
     *
     * Derives from Invocable so the condensed graph can itself be scheduled
     * (invoke walks the component groups in topological order).
     */
    class GraphAnalysis final : public Invocable
    {
    public:
        ssp4cpp::utils::log::Logger *log = nullptr;

        // The original nodes the analysis operates on.
        std::vector<Invocable *> nodes;

        // SCCs computed from the graph, each is a vector of nodes.
        std::vector<std::vector<Invocable *>> sccs;

        // One SccGroup wrapper per SCC; index i matches sccs[i].
        std::vector<std::unique_ptr<SccGroup>> groups;

        // Topologically ordered indices into sccs/groups.
        std::vector<std::size_t> execution_order;

        // For each node, its SCC index.
        std::map<Invocable *, std::size_t> node_to_scc;

        explicit GraphAnalysis(std::vector<Invocable *> nodes);

        // Run the full analysis pipeline: SCC detection, DAG build,
        // topological sort and condensed graph construction.
        void analyze();

        std::string to_string() const override;

        // Walk the condensed graph in topological order, invoking each group.
        uint64_t invoke(StepData step_data) override final;

        // ---- Analysis helpers (also public for reuse) ---------------------

        // Find strongly connected components of the given nodes.
        static std::vector<std::vector<Invocable *>> strongly_connected_components(
            const std::vector<Invocable *> &nodes);

        // Build the map from source SCC index to target SCC indices.
        std::map<std::size_t, std::set<std::size_t>> build_scc_dag() const;

        // Kahn's topological sorting of the SCC DAG.
        std::vector<std::size_t> topological_sort(
            const std::map<std::size_t, std::set<std::size_t>> &dag) const;

        // Wrap each SCC in an SccGroup and rewire the children/parents so the
        // groups form the equivalent condensed DAG.
        void build_condensed_graph();

        // Verify parent/child placement in the graph in relation to a node.
        // A placement is valid when every edge always points forward in the
        // topological order, i.e. for every child c of node,
        // order[scc(node)] <= order[scc(c)], and symmetrically for parents.
        bool verify_placement() const;

        // SCC index of a node (requires analyze() to have run).
        std::size_t scc_index_of(Invocable *node) const;

        // The SccGroup wrapper containing a node (requires analyze()).
        // NOTE: the returned pointer is owned by this GraphAnalysis and is
        // invalidated by any later call to build_condensed_graph() (which
        // clears `groups`) or by destruction of this object. Use it only
        // after analyze() and before the next build_condensed_graph().
        SccGroup *group_of(Invocable *node) const;
    };

}