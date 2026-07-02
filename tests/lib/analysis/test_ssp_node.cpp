#include "pre/2_analysis/elements/ssp_node.hpp"

#include <catch2/catch_test_macros.hpp>

#include <map>
#include <set>
#include <string>
#include <vector>

using ssp4sim::analysis::SspConnector;
using ssp4sim::analysis::SspConnectorNode;
using ssp4sim::analysis::SspConnection;
using ssp4sim::analysis::SspConnectionNode;
using ssp4sim::analysis::SspNode;
using ssp4sim::analysis::SspSystem;
using ssp4sim::analysis::SspSystemNode;
using ssp4sim::types::Causality;
using ssp4sim::types::DataType;

// ---------------------------------------------------------------------------
// Description: Verifies SspNode::as<U>() returns correct typed source pointer
// Rationale:   Type-safe downcasting is the primary analysis tree access pattern
// ---------------------------------------------------------------------------
TEST_CASE("SspNode::as<U>() casts source to correct type", "[ssp_node]")
{
    SspConnector conn("test.conn", 42, DataType::real, Causality::input);
    SspConnectorNode node(&conn);

    SECTION("Correct cast succeeds")
    {
        auto *result = node.as<SspConnector>();
        REQUIRE(result != nullptr);
        CHECK(result->name == "test.conn");
        CHECK(result->value_reference == 42);
    }
}

// ---------------------------------------------------------------------------
// Description: Verifies get_child_nodes<U>() filters children by type
// Rationale:   Typed child iteration is the primary tree traversal mechanism
// ---------------------------------------------------------------------------
TEST_CASE("SspNode::get_child_nodes<U>() filters children by type", "[ssp_node]")
{
    SspConnector conn_a("model.in", 1, DataType::real, Causality::input);
    SspConnector conn_b("model.out", 2, DataType::real, Causality::output);
    SspConnection edge("src", "out", "tgt", "in");

    SspConnectorNode node_a(&conn_a);
    SspConnectorNode node_b(&conn_b);
    SspConnectionNode node_edge(&edge);

    // Use a connector node as parent (SspConnector has a simple constructor)
    SspConnector parent_conn("parent", 0, DataType::real, Causality::parameter);
    SspConnectorNode parent(&parent_conn);
    parent.add_child(&node_a);
    parent.add_child(&node_b);
    parent.add_child(&node_edge);

    SECTION("Filters to connector nodes only")
    {
        auto connectors = parent.get_child_nodes<SspConnectorNode>();
        REQUIRE(connectors.size() == 2);
        // Order should match insertion
        CHECK(connectors[0]->source->name == "model.in");
        CHECK(connectors[1]->source->name == "model.out");
    }

    SECTION("Filters to connection nodes only")
    {
        auto connections = parent.get_child_nodes<SspConnectionNode>();
        REQUIRE(connections.size() == 1);
        CHECK(connections[0]->source->name.find("src.out->tgt.in") != std::string::npos);
    }

    SECTION("Empty result when no children match")
    {
        SspConnector empty_parent_conn("empty", 0, DataType::real, Causality::parameter);
        SspConnectorNode empty_parent(&empty_parent_conn);
        auto connectors = empty_parent.get_child_nodes<SspConnectorNode>();
        CHECK(connectors.empty());
    }
}

// ---------------------------------------------------------------------------
// Description: Verifies add_child establishes bidirectional parent/child linkage
// Rationale:   Bidirectional linkage required for graph traversal both ways
// ---------------------------------------------------------------------------
TEST_CASE("SspNode add_child establishes bidirectional linkage", "[ssp_node]")
{
    SspConnector conn("model.out", 1, DataType::real, Causality::output);
    SspConnectorNode child(&conn);
    SspConnector parent_conn("parent", 0, DataType::real, Causality::parameter);
    SspConnectorNode parent(&parent_conn);

    SECTION("add_child adds to children and sets parent")
    {
        parent.add_child(&child);
        CHECK(parent.nr_children() == 1);
        CHECK(parent.children[0] == &child);
        CHECK(child.nr_parents() == 1);
        CHECK(child.parents[0] == &parent);
    }

    SECTION("Multiple children tracked correctly")
    {
        SspConnector conn2("model.in", 2, DataType::real, Causality::input);
        SspConnectorNode child2(&conn2);

        parent.add_child(&child);
        parent.add_child(&child2);

        CHECK(parent.nr_children() == 2);
        CHECK(child.parents[0] == &parent);
        CHECK(child2.parents[0] == &parent);
    }
}

// ---------------------------------------------------------------------------
// Description: Verifies node.name matches source.name
// Rationale:   Name propagation is the primary identification mechanism
// ---------------------------------------------------------------------------
TEST_CASE("SspNode name is propagated from source", "[ssp_node]")
{
    SspConnector conn("sine.amplitude", 1, DataType::real, Causality::parameter);
    SspConnectorNode node(&conn);

    CHECK(node.name == "sine.amplitude");
    CHECK(node.source->name == "sine.amplitude");
}

// ---------------------------------------------------------------------------
// Description: Verifies SspConnectionNode wraps source_model, target_model,
//              delay, is_boundary correctly
// Rationale:   Connection node wrapping must preserve all connection fields
// ---------------------------------------------------------------------------
TEST_CASE("SspNode<SspConnection> wraps connection correctly", "[ssp_node]")
{
    SspConnection conn("src_model", "src_conn", "tgt_model", "tgt_conn");
    conn.delay = 5;
    SspConnectionNode node(&conn);

    CHECK(node.source->source_model == "src_model");
    CHECK(node.source->target_model == "tgt_model");
    CHECK(node.source->delay == 5);
    CHECK(node.source->is_boundary == false);
}

// ---------------------------------------------------------------------------
// Description: Verifies boundary detection via empty source/target model
// Rationale:   Boundary connections are a special case in SSP topology
// ---------------------------------------------------------------------------
TEST_CASE("SspNode<SspConnection> handles boundary connections", "[ssp_node]")
{
    SspConnection boundary("", "sys_in", "model", "input");
    SspConnectionNode node(&boundary);

    CHECK(boundary.is_boundary == true);
    CHECK(node.source->is_boundary == true);
}