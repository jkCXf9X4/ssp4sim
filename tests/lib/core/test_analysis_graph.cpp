#include "graph/analysis/analysis_internal.hpp"
#include "model/model_connection.hpp"
#include "signal/storage.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>

using ssp4sim::analysis::graph::AnalysisModelVariable;
using ssp4sim::graph::ConnectionInfo;
using ssp4sim::signal::SignalStorage;
using ssp4sim::types::DataType;

// ---------------------------------------------------------------------------
// IMP-022: Verify that dependency-to-output edge direction follows the
// convention that parent = upstream, child = downstream.
//
// The get_dependencies_variables() tuple is (output*, depended-on*,
// DependenciesKind).  source = output (downstream), target = depended-on
// (upstream).  The fix calls  target_node->add_child(source_node)  so that
// the upstream node becomes parent and the downstream node becomes child.
// ---------------------------------------------------------------------------
TEST_CASE("IMP-022 dependency edge direction: upstream becomes parent, downstream becomes child",
          "[analysis_graph]")
{
    // Simulate the fixed pattern: target (depended-on / upstream) adds
    // source (output / downstream) as a child.
    AnalysisModelVariable upstream("MyFmu", "depended_on_var");
    AnalysisModelVariable downstream("MyFmu", "output_var");

    // This is the corrected call from the fix
    upstream.add_child(&downstream);

    // upstream (depended-on) should have downstream as child
    REQUIRE(upstream.children.size() == 1);
    REQUIRE(upstream.children[0] == &downstream);

    // downstream (output) should have upstream as parent
    REQUIRE(downstream.parents.size() == 1);
    REQUIRE(downstream.parents[0] == &upstream);

    // Pre-order traversal should visit upstream before downstream
    std::vector<std::string> traversal_order;
    for (auto *n : upstream)
    {
        traversal_order.push_back(n->name);
    }
    REQUIRE(traversal_order.size() == 2);
    REQUIRE(traversal_order[0] == upstream.name);
    REQUIRE(traversal_order[1] == downstream.name);
}

// ---------------------------------------------------------------------------
// IMP-023: Verify that is_feedthrough round-trips through
// ConnectionInfo::to_string().
// ---------------------------------------------------------------------------
TEST_CASE("IMP-023 feedthrough flag round-trips through to_string",
          "[analysis_graph]")
{
    SignalStorage source_storage(2, "source_storage");
    SignalStorage target_storage(2, "target_storage");

    ConnectionInfo conn;
    conn.type = DataType::real;
    conn.size = 1;
    conn.source_storage = &source_storage;
    conn.target_storage = &target_storage;
    conn.source_index = 0;
    conn.target_index = 0;
    conn.is_feedthrough = true;

    std::string s = conn.to_string();
    CAPTURE(s);
    REQUIRE(s.find("is_feedthrough: 1") != std::string::npos);

    // Also verify that a non-feedthrough connection shows 0
    ConnectionInfo non_feedthrough;
    non_feedthrough.type = DataType::integer;
    non_feedthrough.size = 2;
    non_feedthrough.source_storage = &source_storage;
    non_feedthrough.target_storage = &target_storage;
    non_feedthrough.source_index = 0;
    non_feedthrough.target_index = 0;
    non_feedthrough.is_feedthrough = false;

    std::string ns = non_feedthrough.to_string();
    CAPTURE(ns);
    REQUIRE(ns.find("is_feedthrough: 0") != std::string::npos);
}