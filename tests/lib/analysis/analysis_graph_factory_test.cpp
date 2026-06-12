#include "analysis/analysis_graph_factory.hpp"
#include "analysis/components/analysis_system.hpp"
#include "analysis/components/analysis_model.hpp"
#include "analysis/components/analysis_connector.hpp"
#include "analysis/components/analysis_connection.hpp"
#include "analysis/components/analysis_model_variable.hpp"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <vector>

using ssp4sim::analysis::AnalysisSystem;
using ssp4sim::analysis::AnalysisModel;
using ssp4sim::analysis::AnalysisConnector;
using ssp4sim::analysis::AnalysisConnection;
using ssp4sim::analysis::AnalysisModelVariable;
using ssp4sim::analysis::AnalysisGraphFactory;
using ssp4sim::analysis::AnalysisNode;
using ssp4sim::analysis::ModelNode;
using ssp4sim::analysis::ConnectorNode;
using ssp4sim::analysis::ResolvedConnectionEntry;
using ssp4sim::analysis::AnalysisGraphData;
using ssp4sim::types::Causality;
using ssp4sim::types::DataType;

// ---------------------------------------------------------------------------
// AnalysisNode<T> typed node construction
// ---------------------------------------------------------------------------
TEST_CASE("AnalysisNode<AnalysisModel> construction", "[analysis_node][typed]")
{
    AnalysisModel model("test_model", "test.fmu", nullptr);
    auto node = std::make_unique<ModelNode>(model.name, true, &model);
    REQUIRE(node->name == "test_model");
    REQUIRE(node->source == &model);
    REQUIRE(node->model == nullptr);
    REQUIRE(node->source->name == "test_model");
}

TEST_CASE("AnalysisNode<AnalysisConnector> construction with model", "[analysis_node][typed]")
{
    AnalysisModel model("motor", "motor.fmu", nullptr);
    AnalysisConnector conn("motor", "V_in", 42, DataType::real);
    conn.name = "motor.V_in";

    auto node = std::make_unique<ConnectorNode>(conn.name, true, &conn, &model);
    REQUIRE(node->name == "motor.V_in");
    REQUIRE(node->source == &conn);
    REQUIRE(node->model == &model);
    REQUIRE(node->source->value_reference == 42);
    REQUIRE(node->model->name == "motor");
}

TEST_CASE("AnalysisNodeT default constructor", "[analysis_node][typed]")
{
    AnalysisNode<AnalysisModel> node;
    REQUIRE(node.source == nullptr);
    REQUIRE(node.model == nullptr);
    REQUIRE(node.name.empty());
}

// ---------------------------------------------------------------------------
// Connector placement invariants
// ---------------------------------------------------------------------------
TEST_CASE("validate_connector_placement on empty system produces no warnings", "[connector_placement]")
{
    AnalysisSystem sys("empty");
    // Should not throw or crash
    sys.validate_connector_placement();
}

TEST_CASE("validate_connector_placement system connectors are boundary", "[connector_placement]")
{
    AnalysisSystem sys("sys");
    auto conn = std::make_unique<AnalysisConnector>("sys", "in", 1, DataType::real);
    conn->is_boundary = true;
    sys.connectors.push_back(std::move(conn));
    // All system connectors are boundary — should pass
    sys.validate_connector_placement();
}

TEST_CASE("validate_connector_placement model connectors are not boundary", "[connector_placement]")
{
    AnalysisSystem sys("sys");
    auto model = std::make_unique<AnalysisModel>("motor", "motor.fmu", nullptr);
    auto conn = std::make_unique<AnalysisConnector>("motor", "V_in", 42, DataType::real);
    conn->is_boundary = false;
    model->connectors.push_back(std::move(conn));
    sys.models.push_back(std::move(model));
    // All model connectors are non-boundary — should pass
    sys.validate_connector_placement();
}

// ---------------------------------------------------------------------------
// Two-graph factory output: model graph
// ---------------------------------------------------------------------------
TEST_CASE("build_model_graph on empty system produces no nodes", "[analysis_graph_factory][model_graph]")
{
    AnalysisSystem sys("empty");
    AnalysisGraphFactory factory(sys);
    auto nodes = factory.build_model_graph();
    REQUIRE(nodes.empty());
}

TEST_CASE("build_model_graph with unconnected models produces isolated nodes", "[analysis_graph_factory][model_graph]")
{
    AnalysisSystem sys("sys");
    sys.models.push_back(std::make_unique<AnalysisModel>("m1", "m1.fmu", nullptr));
    sys.models.push_back(std::make_unique<AnalysisModel>("m2", "m2.fmu", nullptr));

    AnalysisGraphFactory factory(sys);
    auto nodes = factory.build_model_graph();
    REQUIRE(nodes.size() == 2);
    // No connections means no edges
    for (auto &node : nodes)
    {
        REQUIRE(node->children.empty());
        REQUIRE(node->source != nullptr);
    }
}

TEST_CASE("build_model_graph with flat connection creates edge", "[analysis_graph_factory][model_graph]")
{
    AnalysisSystem sys("sys");
    auto m1 = std::make_unique<AnalysisModel>("source", "source.fmu", nullptr);
    auto m2 = std::make_unique<AnalysisModel>("sink", "sink.fmu", nullptr);

    // Add connectors for source model
    auto out_conn = std::make_unique<AnalysisConnector>("source", "out", 1, DataType::real);
    m1->connectors.push_back(std::move(out_conn));

    // Add connectors for sink model
    auto in_conn = std::make_unique<AnalysisConnector>("sink", "in", 2, DataType::real);
    m2->connectors.push_back(std::move(in_conn));

    // Store model pointers before moving
    auto *m1_ptr = m1.get();
    auto *m2_ptr = m2.get();
    sys.models.push_back(std::move(m1));
    sys.models.push_back(std::move(m2));

    // Create connection: source.out -> sink.in
    auto conn = std::make_unique<AnalysisConnection>("source", "out", "sink", "in");
    sys.connections.push_back(std::move(conn));

    AnalysisGraphFactory factory(sys);
    auto nodes = factory.build_model_graph();
    REQUIRE(nodes.size() == 2);

    // Find source and sink nodes
    ModelNode *source_node = nullptr;
    ModelNode *sink_node = nullptr;
    for (auto &node : nodes)
    {
        if (node->source == m1_ptr) source_node = node.get();
        if (node->source == m2_ptr) sink_node = node.get();
    }
    REQUIRE(source_node != nullptr);
    REQUIRE(sink_node != nullptr);

    // source -> sink edge (source has sink as child)
    REQUIRE(source_node->has_child() == true);
}

// ---------------------------------------------------------------------------
// Two-graph factory output: connector graph
// ---------------------------------------------------------------------------
TEST_CASE("build_connector_graph on empty system produces no nodes", "[analysis_graph_factory][connector_graph]")
{
    AnalysisSystem sys("empty");
    AnalysisGraphFactory factory(sys);
    auto nodes = factory.build_connector_graph();
    REQUIRE(nodes.empty());
}

TEST_CASE("build_connector_graph creates connector nodes with model pointers", "[analysis_graph_factory][connector_graph]")
{
    AnalysisSystem sys("sys");
    auto model = std::make_unique<AnalysisModel>("motor", "motor.fmu", nullptr);
    auto conn = std::make_unique<AnalysisConnector>("motor", "V_in", 42, DataType::real);
    model->connectors.push_back(std::move(conn));
    sys.models.push_back(std::move(model));

    AnalysisGraphFactory factory(sys);
    auto nodes = factory.build_connector_graph();
    REQUIRE(nodes.size() == 1);
    REQUIRE(nodes[0]->source != nullptr);
    REQUIRE(nodes[0]->source->value_reference == 42);
    REQUIRE(nodes[0]->model != nullptr);
    REQUIRE(nodes[0]->model->name == "motor");
}

// ---------------------------------------------------------------------------
// ResolvedConnectionEntry (pre-resolved connections)
// ---------------------------------------------------------------------------
TEST_CASE("ResolvedConnectionEntry stores resolved connection data", "[resolved_connection]")
{
    ResolvedConnectionEntry entry;
    entry.source_model = "source";
    entry.source_connector = "source.out";
    entry.target_model = "sink";
    entry.target_connector = "sink.in";
    entry.delay = 0;

    REQUIRE(entry.source_model == "source");
    REQUIRE(entry.source_connector == "source.out");
    REQUIRE(entry.target_model == "sink");
    REQUIRE(entry.target_connector == "sink.in");
    REQUIRE(entry.delay == 0);
}

// ---------------------------------------------------------------------------
// Build all (combined graph data)
// ---------------------------------------------------------------------------
TEST_CASE("build_all on empty system produces empty data", "[analysis_graph_factory][build_all]")
{
    AnalysisSystem sys("empty");
    AnalysisGraphFactory factory(sys);
    auto data = factory.build_all();
    REQUIRE(data.model_nodes.empty());
    REQUIRE(data.connector_nodes.empty());
    REQUIRE(data.resolved_connections.empty());
}

TEST_CASE("build_all with flat system produces graph data", "[analysis_graph_factory][build_all]")
{
    AnalysisSystem sys("sys");
    auto m1 = std::make_unique<AnalysisModel>("source", "source.fmu", nullptr);
    auto m2 = std::make_unique<AnalysisModel>("sink", "sink.fmu", nullptr);

    auto out_conn = std::make_unique<AnalysisConnector>("source", "out", 1, DataType::real);
    m1->connectors.push_back(std::move(out_conn));

    auto in_conn = std::make_unique<AnalysisConnector>("sink", "in", 2, DataType::real);
    m2->connectors.push_back(std::move(in_conn));

    sys.models.push_back(std::move(m1));
    sys.models.push_back(std::move(m2));

    auto conn = std::make_unique<AnalysisConnection>("source", "out", "sink", "in");
    sys.connections.push_back(std::move(conn));

    AnalysisGraphFactory factory(sys);
    auto data = factory.build_all();

    REQUIRE(data.model_nodes.size() == 2);
    REQUIRE(data.connector_nodes.size() == 2);
    REQUIRE(data.resolved_connections.size() == 1);

    // Verify resolved connection
    auto &entry = data.resolved_connections[0];
    REQUIRE(entry.source_model == "source");
    REQUIRE(entry.target_model == "sink");
}

// ---------------------------------------------------------------------------
// AnalysisGraphData structure
// ---------------------------------------------------------------------------
TEST_CASE("AnalysisGraphData carries all three collections", "[analysis_graph_data]")
{
    AnalysisGraphData data;
    REQUIRE(data.model_nodes.empty());
    REQUIRE(data.connector_nodes.empty());
    REQUIRE(data.resolved_connections.empty());

    // Verify move semantics work
    data.model_nodes.push_back(std::make_unique<ModelNode>("m1", true));
    data.connector_nodes.push_back(std::make_unique<ConnectorNode>("c1", true));
    data.resolved_connections.push_back({"src", "src.out", "tgt", "tgt.in", 0});

    REQUIRE(data.model_nodes.size() == 1);
    REQUIRE(data.connector_nodes.size() == 1);
    REQUIRE(data.resolved_connections.size() == 1);

    AnalysisGraphData moved(std::move(data));
    REQUIRE(moved.model_nodes.size() == 1);
    REQUIRE(moved.connector_nodes.size() == 1);
    REQUIRE(moved.resolved_connections.size() == 1);
    REQUIRE(data.model_nodes.empty());
}