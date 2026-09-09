#include "graph_analysis/graph_analysis.hpp"

#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdint>
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
    REQUIRE(analysis.groups[loop_idx]->is_loop());

    // The loop must come before its downstream node in the topo order.
    REQUIRE(analysis.execution_order[0] == loop_idx);
    REQUIRE(analysis.execution_order[1] == analysis.scc_index_of(&c));
}

// ---------------------------------------------------------------------------
// Description: The condensed graph rewires SCCs into SccGroup nodes whose
//              edges mirror the component DAG.
// Rationale:   Reusable "equivalent graph" the schedulers operate on.
// ---------------------------------------------------------------------------
TEST_CASE("GraphAnalysis builds a condensed graph of SccGroups", "[graph_analysis]")
{
    std::vector<std::string> calls;
    CountingInvocable a("a", &calls), b("b", &calls), c("c", &calls), d("d", &calls);
    a.add_child(&b);
    b.add_child(&a);   // loop {a, b}
    b.add_child(&c);
    c.add_child(&d);

    std::vector<Invocable *> nodes{&a, &b, &c, &d};
    GraphAnalysis analysis(nodes);
    analysis.analyze();

    REQUIRE(analysis.groups.size() == analysis.sccs.size());

    // a and b share a group; c and d are standalone groups.
    auto *loop_group = analysis.group_of(&a);
    REQUIRE(loop_group == analysis.group_of(&b));
    REQUIRE(loop_group->sub_nodes.size() == 2);

    auto *c_group = analysis.group_of(&c);
    auto *d_group = analysis.group_of(&d);
    REQUIRE(c_group != d_group);

    // Loop group feeds c_group which feeds d_group.
    auto children_of = [](auto *g) {
        std::vector<std::string> names;
        for (auto *child : g->children)
        {
            names.push_back(child->name);
        }
        return names;
    };
    REQUIRE(children_of(loop_group) == std::vector<std::string>{"SccGroup"});
    REQUIRE(children_of(c_group) == std::vector<std::string>{"SccGroup"});
    REQUIRE(d_group->children.empty());

    // Group names are the SCC group default; verify parent wiring back.
    REQUIRE(loop_group->parents.empty());
    REQUIRE(c_group->parents.size() == 1);
    REQUIRE(c_group->parents[0] == loop_group);
    REQUIRE(d_group->parents.size() == 1);
    REQUIRE(d_group->parents[0] == c_group);
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
// Description: invoke walks the condensed graph in topo order, invoking each
//              group's sub-nodes.
// Rationale:   GraphAnalysis is itself an Invocable usable as a scheduler.
// ---------------------------------------------------------------------------
TEST_CASE("GraphAnalysis invoke runs components in topological order", "[graph_analysis]")
{
    std::vector<std::string> calls;
    CountingInvocable a("a", &calls), b("b", &calls), c("c", &calls);
    a.add_child(&b);
    b.add_child(&a);   // loop {a, b}
    b.add_child(&c);

    std::vector<Invocable *> nodes{&a, &b, &c};
    GraphAnalysis analysis(nodes);
    analysis.analyze();

    auto end_time = analysis.invoke(ssp4sim::graph::StepData(0, 10, 10));

    REQUIRE(end_time == 10);
    REQUIRE(calls.size() == 3);
    REQUIRE(std::find(calls.begin(), calls.end(), "a") != calls.end());
    REQUIRE(std::find(calls.begin(), calls.end(), "b") != calls.end());
    REQUIRE(calls.back() == "c");
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
    REQUIRE(analysis.groups.empty());
    REQUIRE(analysis.verify_placement());
}

// ---------------------------------------------------------------------------
// Description: a single acyclic node is not a loop.
// Rationale:   is_loop() must return false for a genuinely acyclic node so
//              schedulers do not relax it as a feedback loop.
// ---------------------------------------------------------------------------
TEST_CASE("SccGroup single acyclic node is not a loop", "[graph_analysis]")
{
    std::vector<std::string> calls;
    CountingInvocable a("a", &calls);

    std::vector<Invocable *> nodes{&a};
    GraphAnalysis analysis(nodes);
    analysis.analyze();

    REQUIRE(analysis.sccs.size() == 1);
    REQUIRE(analysis.sccs[0].size() == 1);
    REQUIRE_FALSE(analysis.groups[0]->is_loop());
}

// ---------------------------------------------------------------------------
// Description: a single node with a self-edge is classified as a loop.
// Rationale:   FINDING F: a size-1 SCC with a feedback self-reference must be
//              treated as a loop, not as a plain acyclic node.
// ---------------------------------------------------------------------------
TEST_CASE("SccGroup single-node self-loop is a loop", "[graph_analysis]")
{
    std::vector<std::string> calls;
    CountingInvocable a("a", &calls);
    a.add_child(&a); // feedback self-reference

    std::vector<Invocable *> nodes{&a};
    GraphAnalysis analysis(nodes);
    analysis.analyze();

    REQUIRE(analysis.sccs.size() == 1);
    REQUIRE(analysis.sccs[0].size() == 1);
    REQUIRE(analysis.groups[0]->is_loop());
}

// ---------------------------------------------------------------------------
// Description: a two-node cycle forms one SCC of size 2 that is a loop.
// Rationale:   Multi-node SCCs must remain loops regardless of the
//              single-node self-loop fix.
// ---------------------------------------------------------------------------
TEST_CASE("SccGroup two-node cycle is a loop", "[graph_analysis]")
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
    REQUIRE(analysis.groups[0]->is_loop());
}

// ---------------------------------------------------------------------------
// Description: a cycle in the SCC DAG makes topological_sort throw.
// Rationale:   FINDING G: a truncated topological order silently dropped
//              components; the SCC condensation is a DAG by construction, so
//              a cycle is a structural error and must fail loudly.
// ---------------------------------------------------------------------------
TEST_CASE("GraphAnalysis topological_sort throws on SCC DAG cycle", "[graph_analysis]")
{
    std::vector<std::string> calls;
    CountingInvocable a("a", &calls), b("b", &calls);
    a.add_child(&b);
    b.add_child(&a); // {a, b} is one SCC -> DAG has a single node, no cycle

    std::vector<Invocable *> nodes{&a, &b};
    GraphAnalysis analysis(nodes);
    analysis.analyze();

    // The SCC condensation is a DAG by construction, so a genuine cycle can
    // only be injected by mutating the graph after analysis. Simulate the
    // structural error by feeding a cyclic DAG directly to topological_sort.
    std::map<std::size_t, std::set<std::size_t>> cyclic_dag{
        {0, {1}},
        {1, {0}},
    };
    REQUIRE_THROWS_AS(analysis.topological_sort(cyclic_dag), std::runtime_error);
}
