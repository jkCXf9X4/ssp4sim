#include "graph_analysis/graph_analysis.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace
{
    class CountingInvocable final : public ssp4sim::graph::Invocable
    {
    public:
        std::vector<std::string> *calls = nullptr;

        explicit CountingInvocable(std::string name, std::vector<std::string> *calls)
            : calls(calls)
        {
            this->name = std::move(name);
        }

        std::uint64_t invoke(ssp4sim::graph::StepData step_data) override
        {
            if (calls)
            {
                calls->push_back(name);
            }
            return step_data.end_time;
        }
    };

    using ssp4sim::graph::GraphAnalysis;
    using ssp4sim::graph::Invocable;
} // anonymous namespace

// ---------------------------------------------------------------------------
// Description: SCC detection finds single-node SCCs for a plain DAG.
// Rationale:   Core analysis contract; the building block for schedulers.
// ---------------------------------------------------------------------------
TEST_CASE("GraphAnalysis finds single-node SCCs for a plain DAG", "[graph_analysis]")
{
    std::vector<std::string> calls;
    CountingInvocable a("a", &calls), b("b", &calls), c("c", &calls);
    a.add_child(&b);
    b.add_child(&c);

    std::vector<Invocable *> nodes{&a, &b, &c};
    GraphAnalysis analysis(nodes);
    analysis.analyze();

    REQUIRE(analysis.sccs.size() == 3);
    REQUIRE(analysis.execution_order.size() == 3);
    REQUIRE(analysis.verify_placement());

    // Single-node SCCs in a plain DAG are not loops.
    REQUIRE(analysis.is_loop.size() == 3);
    REQUIRE_FALSE(analysis.is_loop[analysis.scc_index_of(&a)]);
    REQUIRE_FALSE(analysis.is_loop[analysis.scc_index_of(&b)]);
    REQUIRE_FALSE(analysis.is_loop[analysis.scc_index_of(&c)]);

    // a before b before c in execution order.
    auto ra = analysis.scc_index_of(&a);
    auto rb = analysis.scc_index_of(&b);
    auto rc = analysis.scc_index_of(&c);
    REQUIRE(ra != rb);
    REQUIRE(rb != rc);
    REQUIRE(ra != rc);
    REQUIRE(analysis.execution_order[0] == ra);
    REQUIRE(analysis.execution_order[1] == rb);
    REQUIRE(analysis.execution_order[2] == rc);
}

// ---------------------------------------------------------------------------
// Description: SCC detection merges a 2-cycle into one multi-node SCC.
// Rationale:   Loop identification is the main reason schedulers need SCCs.
// ---------------------------------------------------------------------------
TEST_CASE("GraphAnalysis groups a feedback loop into one SCC", "[graph_analysis]")
{
    std::vector<std::string> calls;
    CountingInvocable a("a", &calls), b("b", &calls), c("c", &calls);
    a.add_child(&b);
    b.add_child(&a);   // forms loop {a, b}
    b.add_child(&c);   // c downstream of the loop

    std::vector<Invocable *> nodes{&a, &b, &c};
    GraphAnalysis analysis(nodes);
    analysis.analyze();

    REQUIRE(analysis.sccs.size() == 2);
    REQUIRE(analysis.verify_placement());

    auto loop_idx = analysis.scc_index_of(&a);
    REQUIRE(loop_idx == analysis.scc_index_of(&b));
    REQUIRE(loop_idx != analysis.scc_index_of(&c));
    REQUIRE(analysis.sccs[loop_idx].size() == 2);
    REQUIRE(analysis.is_loop[loop_idx]);

    // The loop must come before its downstream node in the topo order.
    REQUIRE(analysis.execution_order[0] == loop_idx);
    REQUIRE(analysis.execution_order[1] == analysis.scc_index_of(&c));
}

// ---------------------------------------------------------------------------
// Description: verify_placement detects an edge that violates topo order.
// Rationale:   Sanity-check hook for scheduler after graph mutations.
// ---------------------------------------------------------------------------
TEST_CASE("GraphAnalysis verify_placement flags backward edges", "[graph_analysis]")
{
    std::vector<std::string> calls;
    CountingInvocable a("a", &calls), b("b", &calls), c("c", &calls);
    a.add_child(&b);
    b.add_child(&c);

    std::vector<Invocable *> nodes{&a, &b, &c};
    GraphAnalysis analysis(nodes);
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
TEST_CASE("GraphAnalysis handles an empty graph", "[graph_analysis]")
{
    std::vector<Invocable *> nodes{};
    GraphAnalysis analysis(nodes);
    analysis.analyze();

    REQUIRE(analysis.sccs.empty());
    REQUIRE(analysis.execution_order.empty());
    REQUIRE(analysis.is_loop.empty());
    REQUIRE(analysis.verify_placement());
}

// ---------------------------------------------------------------------------
// Description: a single acyclic node is not a loop.
// Rationale:   is_loop must return false for a genuinely acyclic node so
//              schedulers do not relax it as a feedback loop.
// ---------------------------------------------------------------------------
TEST_CASE("GraphAnalysis single acyclic node is not a loop", "[graph_analysis]")
{
    std::vector<std::string> calls;
    CountingInvocable a("a", &calls);

    std::vector<Invocable *> nodes{&a};
    GraphAnalysis analysis(nodes);
    analysis.analyze();

    REQUIRE(analysis.sccs.size() == 1);
    REQUIRE(analysis.sccs[0].size() == 1);
    REQUIRE_FALSE(analysis.is_loop[0]);
}

// ---------------------------------------------------------------------------
// Description: a single node with a self-edge is classified as a loop.
// Rationale:   FINDING F: a size-1 SCC with a feedback self-reference must be
//              treated as a loop, not as a plain acyclic node.
// ---------------------------------------------------------------------------
TEST_CASE("GraphAnalysis single-node self-loop is a loop", "[graph_analysis]")
{
    std::vector<std::string> calls;
    CountingInvocable a("a", &calls);
    a.add_child(&a); // feedback self-reference

    std::vector<Invocable *> nodes{&a};
    GraphAnalysis analysis(nodes);
    analysis.analyze();

    REQUIRE(analysis.sccs.size() == 1);
    REQUIRE(analysis.sccs[0].size() == 1);
    REQUIRE(analysis.is_loop[0]);
}

// ---------------------------------------------------------------------------
// Description: a two-node cycle forms one SCC of size 2 that is a loop.
// Rationale:   Multi-node SCCs must remain loops regardless of the
//              single-node self-loop fix.
// ---------------------------------------------------------------------------
TEST_CASE("GraphAnalysis two-node cycle is a loop", "[graph_analysis]")
{
    std::vector<std::string> calls;
    CountingInvocable a("a", &calls), b("b", &calls);
    a.add_child(&b);
    b.add_child(&a); // forms loop {a, b}

    std::vector<Invocable *> nodes{&a, &b};
    GraphAnalysis analysis(nodes);
    analysis.analyze();

    REQUIRE(analysis.sccs.size() == 1);
    REQUIRE(analysis.sccs[0].size() == 2);
    REQUIRE(analysis.is_loop[0]);
}

// ---------------------------------------------------------------------------
// Description: a cycle in the SCC DAG makes topological_sort throw.
// Rationale:   FINDING G: a truncated topological order silently dropped
//              components; the SCC condensation is a DAG by construction, so
//              a cycle is a structural error and must fail loudly.
// ---------------------------------------------------------------------------
TEST_CASE("GraphAnalysis topological_sort throws on SCC DAG cycle", "[graph_analysis]")
{
    // The SCC condensation is a DAG by construction, so a genuine cycle can
    // only be injected by mutating the graph after analysis. Simulate the
    // structural error by feeding a cyclic DAG directly to topological_sort.
    std::map<std::size_t, std::set<std::size_t>> cyclic_dag{
        {0, {1}},
        {1, {0}},
    };
    REQUIRE_THROWS_AS(
        ssp4sim::graph::GraphAnalysis::topological_sort(cyclic_dag),
        std::runtime_error);
}