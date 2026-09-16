#include "utils/graph/graph.hpp"
#include "utils/graph/rewire.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
    using ssp4sim::utils::graph::Graph;
    using ssp4sim::utils::graph::Node;
} // anonymous namespace

// ---------------------------------------------------------------------------
// Description: SCC detection finds single-node SCCs for a plain DAG.
// Rationale:   Core analysis contract; the building block for schedulers.
// ---------------------------------------------------------------------------
TEST_CASE("Graph finds single-node SCCs for a plain DAG", "[graph]")
{
    Node a("a"), b("b"), c("c");
    a.add_child(&b);
    b.add_child(&c);

    Graph analysis({&a, &b, &c});
    analysis.analyze();

    REQUIRE(analysis.sccs().size() == 3);
    REQUIRE(analysis.execution_order().size() == 3);
    REQUIRE(analysis.verify_placement());

    // Single-node SCCs in a plain DAG are not loops.
    REQUIRE(analysis.is_loop().size() == 3);
    REQUIRE_FALSE(analysis.is_loop()[analysis.scc_index_of(&a)]);
    REQUIRE_FALSE(analysis.is_loop()[analysis.scc_index_of(&b)]);
    REQUIRE_FALSE(analysis.is_loop()[analysis.scc_index_of(&c)]);

    // a before b before c in execution order.
    auto ra = analysis.scc_index_of(&a);
    auto rb = analysis.scc_index_of(&b);
    auto rc = analysis.scc_index_of(&c);
    REQUIRE(ra != rb);
    REQUIRE(rb != rc);
    REQUIRE(ra != rc);
    REQUIRE(analysis.execution_order()[0] == ra);
    REQUIRE(analysis.execution_order()[1] == rb);
    REQUIRE(analysis.execution_order()[2] == rc);
}

// ---------------------------------------------------------------------------
// Description: SCC detection merges a 2-cycle into one multi-node SCC.
// Rationale:   Loop identification is the main reason schedulers need SCCs.
// ---------------------------------------------------------------------------
TEST_CASE("Graph groups a feedback loop into one SCC", "[graph]")
{
    Node a("a"), b("b"), c("c");
    a.add_child(&b);
    b.add_child(&a);   // forms loop {a, b}
    b.add_child(&c);   // c downstream of the loop

    Graph analysis({&a, &b, &c});
    analysis.analyze();

    REQUIRE(analysis.sccs().size() == 2);
    REQUIRE(analysis.verify_placement());

    auto loop_idx = analysis.scc_index_of(&a);
    REQUIRE(loop_idx == analysis.scc_index_of(&b));
    REQUIRE(loop_idx != analysis.scc_index_of(&c));
    REQUIRE(analysis.sccs()[loop_idx].size() == 2);
    REQUIRE(analysis.is_loop()[loop_idx]);

    // The loop must come before its downstream node in the topo order.
    REQUIRE(analysis.execution_order()[0] == loop_idx);
    REQUIRE(analysis.execution_order()[1] == analysis.scc_index_of(&c));
}

// ---------------------------------------------------------------------------
// Description: verify_placement detects an edge that violates topo order.
// Rationale:   Sanity-check hook for scheduler after graph mutations.
// ---------------------------------------------------------------------------
TEST_CASE("Graph verify_placement flags backward edges", "[graph]")
{
    Node a("a"), b("b"), c("c");
    a.add_child(&b);
    b.add_child(&c);

    Graph analysis({&a, &b, &c});
    analysis.analyze();

    REQUIRE(analysis.verify_placement());

    // Force a backward edge: c (last) now feeds a (first). Condensed rank is
    // no longer forward-only, so placement must be rejected.
    c.add_child(&a);
    REQUIRE_FALSE(analysis.verify_placement());
}

// ---------------------------------------------------------------------------
// Description: analyze() and topological_sort() tolerate an empty node set.
// Rationale:   Schedulers may be constructed with no nodes; the analysis must
//              not crash and must produce an empty execution order.
// ---------------------------------------------------------------------------
TEST_CASE("Graph handles an empty graph", "[graph]")
{
    std::vector<Node *> nodes{};
    Graph analysis(nodes);
    analysis.analyze();

    REQUIRE(analysis.sccs().empty());
    REQUIRE(analysis.execution_order().empty());
    REQUIRE(analysis.is_loop().empty());
    REQUIRE(analysis.verify_placement());
}

// ---------------------------------------------------------------------------
// Description: a single acyclic node is not a loop.
// Rationale:   is_loop must return false for a genuinely acyclic node so
//              schedulers do not relax it as a feedback loop.
// ---------------------------------------------------------------------------
TEST_CASE("Graph single acyclic node is not a loop", "[graph]")
{
    Node a("a");

    Graph analysis({&a});
    analysis.analyze();

    REQUIRE(analysis.sccs().size() == 1);
    REQUIRE(analysis.sccs()[0].size() == 1);
    REQUIRE_FALSE(analysis.is_loop()[0]);
}

// ---------------------------------------------------------------------------
// Description: a single node with a self-edge is classified as a loop.
// Rationale:   FINDING F: a size-1 SCC with a feedback self-reference must be
//              treated as a loop, not as a plain acyclic node.
// ---------------------------------------------------------------------------
TEST_CASE("Graph single-node self-loop is a loop", "[graph]")
{
    Node a("a");
    a.add_child(&a); // feedback self-reference

    Graph analysis({&a});
    analysis.analyze();

    REQUIRE(analysis.sccs().size() == 1);
    REQUIRE(analysis.sccs()[0].size() == 1);
    REQUIRE(analysis.is_loop()[0]);
}

// ---------------------------------------------------------------------------
// Description: a two-node cycle forms one SCC of size 2 that is a loop.
// Rationale:   Multi-node SCCs must remain loops regardless of the
//              single-node self-loop fix.
// ---------------------------------------------------------------------------
TEST_CASE("Graph two-node cycle is a loop", "[graph]")
{
    Node a("a"), b("b");
    a.add_child(&b);
    b.add_child(&a); // forms loop {a, b}

    Graph analysis({&a, &b});
    analysis.analyze();

    REQUIRE(analysis.sccs().size() == 1);
    REQUIRE(analysis.sccs()[0].size() == 2);
    REQUIRE(analysis.is_loop()[0]);
}

// ---------------------------------------------------------------------------
// Description: the component DAG is exposed, and condensation rewires
// representatives so every cross-SCC edge points at another representative.
// Rationale:   Seidel reuse builds on the condensation utility; the condensed
//              graph is what the outer Seidel traverses.
// ---------------------------------------------------------------------------
TEST_CASE("Graph component_dag drives condense", "[graph]")
{
    Node a("a"), b("b"), c("c"), d("d");
    a.add_child(&b);
    b.add_child(&c);
    c.add_child(&b); // loop {b, c}
    c.add_child(&d);

    Graph analysis({&a, &b, &c, &d});
    analysis.analyze();

    // The component DAG must list every SCC (loop {b,c} sits between two
    // single-node SCCs).
    REQUIRE(analysis.component_dag().size() == 3);

    // Acyclic components keep themselves as representative; the loop gets a
    // single new node that replaces both members.
    Node loop("LOOP");
    std::vector<Node *> representatives(analysis.sccs().size(), nullptr);
    for (std::size_t i = 0; i < analysis.sccs().size(); ++i)
    {
        representatives[i] = analysis.is_loop()[i]
            ? &loop
            : analysis.sccs()[i][0];
    }

    auto rep_of = [&](Node *n)
    {
        return representatives[analysis.scc_index_of(n)];
    };
    auto *rep_a = rep_of(&a);
    auto *rep_loop = rep_of(&b);
    auto *rep_d = rep_of(&d);

    // The two loop members map to the same representative.
    REQUIRE(rep_of(&c) == rep_loop);

    ssp4sim::utils::graph::condense_component_dag(
        analysis.component_dag(), representatives);

    REQUIRE(rep_a->children.size() == 1);
    REQUIRE(rep_a->children[0] == rep_loop);
    REQUIRE(rep_loop->parents.size() == 1);
    REQUIRE(rep_loop->parents[0] == rep_a);
    REQUIRE(rep_loop->children.size() == 1);
    REQUIRE(rep_loop->children[0] == rep_d);
    REQUIRE(rep_d->parents.size() == 1);
    REQUIRE(rep_d->parents[0] == rep_loop);
    REQUIRE(rep_a->parents.empty());
    REQUIRE(rep_d->children.empty());
}

// ---------------------------------------------------------------------------
// Description: a cycle in the SCC DAG makes topological_sort throw.
// Rationale:   FINDING G: a truncated topological order silently dropped
//              components; the SCC condensation is a DAG by construction, so
//              a cycle is a structural error and must fail loudly.
// ---------------------------------------------------------------------------
TEST_CASE("Graph topological_sort throws on SCC DAG cycle", "[graph]")
{
    // The SCC condensation is a DAG by construction, so a genuine cycle can
    // only be injected by mutating the graph after analysis. Simulate the
    // structural error by feeding a cyclic DAG directly to topological_sort.
    std::map<std::size_t, std::set<std::size_t>> cyclic_dag{
        {0, {1}},
        {1, {0}},
    };
    REQUIRE_THROWS_AS(
        ssp4sim::utils::graph::topological_sort(cyclic_dag),
        std::runtime_error);
}

// ---------------------------------------------------------------------------
// Description: disconnect removes a connection in both directions.
// Rationale:   Connection mutation must always stay bidirectional-consistent
//              so a later analysis sees one coherent graph.
// ---------------------------------------------------------------------------
TEST_CASE("rewire disconnect removes the connection both ways", "[rewire]")
{
    Node a("a"), b("b");
    a.add_child(&b);

    REQUIRE(a.contains_child(&b));
    REQUIRE(b.contains_parent(&a));

    ssp4sim::utils::graph::disconnect(&a, &b);

    REQUIRE_FALSE(a.contains_child(&b));
    REQUIRE_FALSE(b.contains_parent(&a));
    REQUIRE(a.is_orphan());
    REQUIRE(b.is_orphan());

    // Disconnecting an absent connection is a no-op.
    ssp4sim::utils::graph::disconnect(&a, &b);
    REQUIRE(a.is_orphan());
}

// ---------------------------------------------------------------------------
// Description: redirect replaces from by to inside the given scope only.
// Rationale:   Graph composition changes rewire a scope of nodes (e.g. after
//              condensation), never touch nodes outside it, and dedupe
//              duplicate connections through Node's guards.
// ---------------------------------------------------------------------------
TEST_CASE("rewire redirect replaces connections across the scope", "[rewire]")
{
    Node a("a"), b("b"), c("c");
    a.add_child(&b);   // a -> b
    b.add_child(&c);   // b -> c (b stays untouched, not in scope)

    // Replace b by c within the scope {a}: a -> c.
    ssp4sim::utils::graph::redirect({&a}, &b, &c);

    REQUIRE_FALSE(a.contains_child(&b));
    REQUIRE(a.contains_child(&c));
    REQUIRE(c.contains_parent(&a));

    // b keeps its own outgoing b -> c edge (b is not in scope).
    REQUIRE(b.contains_child(&c));

    // Replacing a node that is never referenced is a no-op.
    Node d("d");
    ssp4sim::utils::graph::redirect({&a}, &d, &c);
    REQUIRE_FALSE(a.contains_child(&d));
}