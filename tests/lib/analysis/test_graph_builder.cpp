#include "pre/1_ssp_parser/ssp_parser.hpp"
#include "pre/2_analysis/tree_builder.hpp"
#include "pre/2_analysis/graph_builder.hpp"
#include "pre/2_analysis/elements/ssp_node.hpp"
#include "pre/ssp_graph_data.hpp"

#include "ssp4cpp/ssp.hpp"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <set>
#include <string>

namespace fs = std::filesystem;

namespace {

    fs::path project_root()
    {
        return fs::path(SSP4SIM_PROJECT_ROOT);
    }

    fs::path fixture_path(const std::string &fixture_name)
    {
        return project_root() / "resources" / "reference_ssp" / "artifacts" / "models" /
               fixture_name / "baseline";
    }

    /// Build the full analysis graph pipeline from a fixture.
    /// Returns the AnalysisGraphData produced by SspGraphBuilder.
    ssp4sim::analysis::AnalysisGraphData build_graph(const std::string &ssp_path)
    {
        // Step 1: construct Ssp
        ssp4cpp::Ssp ssp(ssp_path);

        // Step 2: build SspSystem
        auto analysis_system = ssp4sim::analysis::SspSystemBuilder().build(&ssp);

        // Step 3: build tree (SspSystemNode)
        ssp4sim::analysis::SspTreeBuilder tree_builder;
        auto *system_tree = tree_builder.build(&analysis_system);

        // Step 4: build graph (AnalysisGraphData)
        ssp4sim::analysis::SspGraphBuilder graph_builder;
        return graph_builder.build(system_tree);
    }

    } // anonymous namespace

// ---------------------------------------------------------------------------
// Test: signal_sine_gain_add — 4 models, 3 connections, flat (no nesting)
// ---------------------------------------------------------------------------
TEST_CASE("SspGraphBuilder builds correct graph structure for sine_gain_add",
          "[graph_builder][analysis]")
{
    auto gd = build_graph(fixture_path("signal_sine_gain_add").string());

    // --- Model count ---
    // sine, step, gain, add
    REQUIRE(gd.model_nodes.size() == 4);

    // --- Connector count ---
    // sine: 5 parameters + 1 output = 6
    // step: 3 parameters + 1 output = 4
    // gain: 1 parameter + 1 input + 1 output = 3
    // add:  2 parameters + 2 inputs + 1 output = 5
    // Total: 18
    REQUIRE(gd.connector_nodes.size() == 18);

    // --- Connection count ---
    // sine.y -> gain.u
    // gain.y -> add.u1
    // step.y -> add.u2
    // Total: 3
    REQUIRE(gd.connection_nodes.size() == 3);
    REQUIRE(gd.connection_sources.size() == 3);

    // --- Verify model names ---
    std::set<std::string> model_names;
    for (const auto &m : gd.model_nodes)
        model_names.insert(m->name);
    CHECK(model_names.count("sine") == 1);
    CHECK(model_names.count("step") == 1);
    CHECK(model_names.count("gain") == 1);
    CHECK(model_names.count("add") == 1);
    CHECK(model_names.size() == 4);

    // --- Verify graph chain structure ---
    // Each model node should have connector children
    for (const auto &m : gd.model_nodes)
    {
        auto connectors = m->get_child_nodes<ssp4sim::analysis::SspConnectorNode>();
        CHECK_FALSE(connectors.empty());
    }

    // --- Verify each connection resolved to a connector-to-connector edge ---
    // Each connection node should have a single target connector child
    for (const auto &c : gd.connection_nodes)
    {
        CHECK(c->source != nullptr);
        // The resolved connection should have a target connector as child
        auto target_connectors = c->get_child_nodes<ssp4sim::analysis::SspConnectorNode>();
        CHECK_FALSE(target_connectors.empty());
    }

    // --- Verify each connector has a parent model ---
    for (const auto &c : gd.connector_nodes)
    {
        bool has_model_parent = false;
        for (auto *parent : c->parents)
        {
            if (dynamic_cast<ssp4sim::analysis::SspModelNode *>(parent))
            {
                has_model_parent = true;
                break;
            }
        }
        // System-level (boundary) connectors may not have a model parent,
        // but all model-level connectors should
        CHECK(has_model_parent);
    }
}

// ---------------------------------------------------------------------------
// Test: signal_fanout_gain — 3 models, 2 connections (fanout pattern)
// ---------------------------------------------------------------------------
TEST_CASE("SspGraphBuilder builds correct graph structure for fanout_gain",
          "[graph_builder][analysis]")
{
    auto gd = build_graph(fixture_path("signal_fanout_gain").string());

    // --- Model count ---
    // step, gain_a, gain_b
    REQUIRE(gd.model_nodes.size() == 3);

    // --- Connector count ---
    // step:   3 parameters + 1 output = 4
    // gain_a: 1 parameter + 1 input + 1 output = 3
    // gain_b: 1 parameter + 1 input + 1 output = 3
    // Total: 10
    REQUIRE(gd.connector_nodes.size() == 10);

    // --- Connection count ---
    // step.y -> gain_a.u
    // step.y -> gain_b.u
    // Total: 2
    REQUIRE(gd.connection_nodes.size() == 2);
    REQUIRE(gd.connection_sources.size() == 2);

    // --- Verify model names ---
    std::set<std::string> model_names;
    for (const auto &m : gd.model_nodes)
        model_names.insert(m->name);
    CHECK(model_names.count("step") == 1);
    CHECK(model_names.count("gain_a") == 1);
    CHECK(model_names.count("gain_b") == 1);
    CHECK(model_names.size() == 3);
}

// ---------------------------------------------------------------------------
// Test: signal_step_gain — 2 models, 1 connection, external SSV
// ---------------------------------------------------------------------------
TEST_CASE("SspGraphBuilder builds correct graph structure for step_gain",
          "[graph_builder][analysis]")
{
    auto gd = build_graph(fixture_path("signal_step_gain").string());

    // --- Model count ---
    // step, gain
    REQUIRE(gd.model_nodes.size() == 2);

    // --- Connector count ---
    // step: 3 parameters + 1 output = 4
    // gain: 1 parameter + 1 input + 1 output = 3
    // Total: 7
    REQUIRE(gd.connector_nodes.size() == 7);

    // --- Connection count ---
    // step.y -> gain.u
    REQUIRE(gd.connection_nodes.size() == 1);
    REQUIRE(gd.connection_sources.size() == 1);

    // --- Verify model names ---
    std::set<std::string> model_names;
    for (const auto &m : gd.model_nodes)
        model_names.insert(m->name);
    CHECK(model_names.count("step") == 1);
    CHECK(model_names.count("gain") == 1);
    CHECK(model_names.size() == 2);
}

// ---------------------------------------------------------------------------
// Test: signal_step_product — 3 models, 2 connections
// ---------------------------------------------------------------------------
TEST_CASE("SspGraphBuilder builds correct graph structure for step_product",
          "[graph_builder][analysis]")
{
    auto gd = build_graph(fixture_path("signal_step_product").string());

    // --- Model count ---
    // step, gain, product
    REQUIRE(gd.model_nodes.size() == 3);

    // --- Connection count ---
    // step.y -> product.u1
    // gain.y -> product.u2
    REQUIRE(gd.connection_nodes.size() == 2);
    REQUIRE(gd.connection_sources.size() == 2);

    // --- Verify model names ---
    std::set<std::string> model_names;
    for (const auto &m : gd.model_nodes)
        model_names.insert(m->name);
    CHECK(model_names.count("step") == 1);
    CHECK(model_names.count("sine") == 1);
    CHECK(model_names.count("product") == 1);
    CHECK(model_names.size() == 3);
}

// ---------------------------------------------------------------------------
// Test: Graph structure integrity — model→connector→connection→connector→model
// ---------------------------------------------------------------------------
TEST_CASE("SspGraphBuilder produces valid graph chain integrity",
          "[graph_builder][analysis]")
{
    auto gd = build_graph(fixture_path("signal_sine_gain_add").string());

    // For each model node, verify that connectors are children
    for (const auto &model_node : gd.model_nodes)
    {
        auto connectors = model_node->get_child_nodes<ssp4sim::analysis::SspConnectorNode>();
        for (auto *conn : connectors)
        {
            // Verify connector is a child of the model
            bool is_child_of_model = false;
            for (auto *parent : conn->parents)
            {
                if (parent == model_node.get())
                {
                    is_child_of_model = true;
                    break;
                }
            }
            CHECK(is_child_of_model);
        }
    }

    // For each connection node, verify it has a connector as child
    for (const auto &conn_node : gd.connection_nodes)
    {
        auto target_connectors = conn_node->get_child_nodes<ssp4sim::analysis::SspConnectorNode>();
        CHECK_FALSE(target_connectors.empty());

        // Verify delay is propagated
        CHECK(conn_node->source != nullptr);
    }
}

// ---------------------------------------------------------------------------
// Test: System with models but no connections
// ---------------------------------------------------------------------------
TEST_CASE("SspGraphBuilder handles models without connections", "[graph_builder][analysis]")
{
    // signal_parameter_inline_with_mapping has 1 model (step) with no connections
    auto gd = build_graph(fixture_path("signal_parameter_inline_with_mapping").string());

    // Only step model
    REQUIRE(gd.model_nodes.size() == 1);

    // step has 4 connectors (height, offset, startTime, y)
    REQUIRE(gd.connector_nodes.size() == 4);

    // No connections
    CHECK(gd.connection_nodes.empty());
    CHECK(gd.connection_sources.empty());
}