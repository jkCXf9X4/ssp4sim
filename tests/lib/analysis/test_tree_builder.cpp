#include "pre/1_ssp_parser/ssp_parser.hpp"
#include "pre/2_analysis/tree_builder.hpp"
#include "pre/2_analysis/elements/ssp_node.hpp"

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

    /// Build the SspSystem from a fixture.
    ssp4sim::analysis::SspSystem build_system(const std::string &ssp_path)
    {
        ssp4cpp::Ssp ssp(ssp_path);
        auto analysis_system = ssp4sim::analysis::SspSystemBuilder().build(&ssp);
        return analysis_system;
    }

    /// Return tree pointer from a system. The caller must keep the SspSystem alive.
    ssp4sim::analysis::SspSystemNode *build_tree_from(
        ssp4sim::analysis::SspSystem *sys,
        ssp4sim::analysis::SspTreeBuilder &builder)
    {
        return builder.build(sys);
    }

} // anonymous namespace

// ---------------------------------------------------------------------------
// Description: Verifies tree hierarchy for sine_gain_add (4 models, 3 conns)
// Rationale:   Core tree structure for flat multi-model SSP
// ---------------------------------------------------------------------------
TEST_CASE("SspTreeBuilder builds correct model hierarchy for sine_gain_add",
          "[tree_builder][analysis]")
{
    auto sys = build_system(fixture_path("signal_sine_gain_add").string());
    ssp4sim::analysis::SspTreeBuilder tree_builder;
    auto *tree = build_tree_from(&sys, tree_builder);
    REQUIRE(tree != nullptr);

    // Root system
    CHECK(tree->name == "system");

    // Count model children at root level
    auto models = tree->get_child_nodes<ssp4sim::analysis::SspModelNode>();
    REQUIRE(models.size() == 4);

    // Verify model names
    std::set<std::string> model_names;
    for (auto *m : models)
        model_names.insert(m->name);
    CHECK(model_names.count("sine") == 1);
    CHECK(model_names.count("step") == 1);
    CHECK(model_names.count("gain") == 1);
    CHECK(model_names.count("add") == 1);

    // Verify each model has connector children
    for (auto *m : models)
    {
        auto connectors = m->get_child_nodes<ssp4sim::analysis::SspConnectorNode>();
        CHECK_FALSE(connectors.empty());
    }

    // Verify connection nodes exist at system level
    auto connections = tree->get_child_nodes<ssp4sim::analysis::SspConnectionNode>();
    REQUIRE(connections.size() == 3);
}

// ---------------------------------------------------------------------------
// Description: Verifies tree hierarchy for fanout_gain (3 models, 2 conns)
// Rationale:   Fanout topology coverage
// ---------------------------------------------------------------------------
TEST_CASE("SspTreeBuilder builds correct hierarchy for fanout_gain",
          "[tree_builder][analysis]")
{
    auto sys = build_system(fixture_path("signal_fanout_gain").string());
    ssp4sim::analysis::SspTreeBuilder tree_builder;
    auto *tree = build_tree_from(&sys, tree_builder);

    auto models = tree->get_child_nodes<ssp4sim::analysis::SspModelNode>();
    REQUIRE(models.size() == 3);

    std::set<std::string> model_names;
    for (auto *m : models)
        model_names.insert(m->name);
    CHECK(model_names.count("step") == 1);
    CHECK(model_names.count("gain_a") == 1);
    CHECK(model_names.count("gain_b") == 1);

    auto connections = tree->get_child_nodes<ssp4sim::analysis::SspConnectionNode>();
    REQUIRE(connections.size() == 2);
}

// ---------------------------------------------------------------------------
// Description: Verifies single model with zero connections
// Rationale:   Edge case — disconnected models must produce valid trees
// ---------------------------------------------------------------------------
TEST_CASE("SspTreeBuilder handles system with no connections",
          "[tree_builder][analysis]")
{
    auto sys = build_system(fixture_path("signal_parameter_inline_with_mapping").string());
    ssp4sim::analysis::SspTreeBuilder tree_builder;
    auto *tree = build_tree_from(&sys, tree_builder);

    auto models = tree->get_child_nodes<ssp4sim::analysis::SspModelNode>();
    REQUIRE(models.size() == 1);
    CHECK(models[0]->name == "step");

    auto connections = tree->get_child_nodes<ssp4sim::analysis::SspConnectionNode>();
    CHECK(connections.empty());
}

// ---------------------------------------------------------------------------
// Description: Verifies connector names include model prefix ("model.")
// Rationale:   Name prefixing required for correct parameter binding lookup
// ---------------------------------------------------------------------------
TEST_CASE("SspTreeBuilder connector names include model prefix",
          "[tree_builder][analysis]")
{
    auto sys = build_system(fixture_path("signal_step_gain").string());
    ssp4sim::analysis::SspTreeBuilder tree_builder;
    auto *tree = build_tree_from(&sys, tree_builder);

    auto models = tree->get_child_nodes<ssp4sim::analysis::SspModelNode>();
    REQUIRE(models.size() == 2);

    // Find the step model and check its connectors
    ssp4sim::analysis::SspModelNode *step_node = nullptr;
    ssp4sim::analysis::SspModelNode *gain_node = nullptr;
    for (auto *m : models)
    {
        if (m->name == "step") step_node = m;
        if (m->name == "gain") gain_node = m;
    }
    REQUIRE(step_node != nullptr);
    REQUIRE(gain_node != nullptr);

    // Connector names should include model prefix (e.g., "step.height", "step.y")
    auto step_connectors = step_node->get_child_nodes<ssp4sim::analysis::SspConnectorNode>();
    for (auto *c : step_connectors)
    {
        CHECK(c->name.find("step.") == 0);
    }

    auto gain_connectors = gain_node->get_child_nodes<ssp4sim::analysis::SspConnectorNode>();
    for (auto *c : gain_connectors)
    {
        CHECK(c->name.find("gain.") == 0);
    }
}

// ---------------------------------------------------------------------------
// Description: Verifies gain.k = 3.0 after parameter binding
// Rationale:   Parameter application is the primary purpose of the tree builder
// ---------------------------------------------------------------------------
TEST_CASE("SspTreeBuilder applies parameters to connector initial_values",
          "[tree_builder][analysis]")
{
    auto sys = build_system(fixture_path("signal_sine_gain_add").string());
    ssp4sim::analysis::SspTreeBuilder tree_builder;
    auto *tree = build_tree_from(&sys, tree_builder);

    auto models = tree->get_child_nodes<ssp4sim::analysis::SspModelNode>();

    // Find the gain model and check its "k" parameter connector
    ssp4sim::analysis::SspModelNode *gain_node = nullptr;
    for (auto *m : models)
    {
        if (m->name == "gain") { gain_node = m; break; }
    }
    REQUIRE(gain_node != nullptr);

    auto gain_connectors = gain_node->get_child_nodes<ssp4sim::analysis::SspConnectorNode>();
    bool found_k = false;
    for (auto *c : gain_connectors)
    {
        if (c->name == "gain.k")
        {
            found_k = true;
            // The parameter binding should have set initial_value to 3.0
            CHECK(c->source->initial_value.type == ssp4sim::types::DataType::real);
            CHECK(std::holds_alternative<double>(c->source->initial_value.value));
            CHECK(std::get<double>(c->source->initial_value.value) == 3.0);
            break;
        }
    }
    CHECK(found_k);
}

// ---------------------------------------------------------------------------
// Description: Verifies node_owner retains all created SspNode wrappers
// Rationale:   Node ownership ensures no dangling pointers
// Creep flag:  Assertion >= 10 is a lower bound; exact count varies by fixture
// ---------------------------------------------------------------------------
TEST_CASE("SspTreeBuilder node_owner retains all nodes",
          "[tree_builder][analysis]")
{
    auto sys = build_system(fixture_path("signal_step_gain").string());
    ssp4sim::analysis::SspTreeBuilder tree_builder;
    auto *tree = build_tree_from(&sys, tree_builder);

    // node_owner retains all created SspNode<T> wrappers.
    // For signal_step_gain: 1 system + 2 models + ~7 connectors + 1 connection
    // = at least 11 nodes. Use >= 10 as a safe lower bound.
    CHECK(tree_builder.node_owner.size() >= 10);
}

// ---------------------------------------------------------------------------
// Description: Verifies connection name format "src.out->tgt.in"
// Rationale:   Connection naming convention for debugging
// Creep flag:  Name format is a presentation detail
// ---------------------------------------------------------------------------
TEST_CASE("SspTreeBuilder connection nodes have descriptive names",
          "[tree_builder][analysis]")
{
    auto sys = build_system(fixture_path("signal_step_gain").string());
    ssp4sim::analysis::SspTreeBuilder tree_builder;
    auto *tree = build_tree_from(&sys, tree_builder);

    auto connections = tree->get_child_nodes<ssp4sim::analysis::SspConnectionNode>();
    REQUIRE(connections.size() == 1);

    // Connection name should be "source_model.source_connector->target_model.target_connector"
    CHECK(connections[0]->name.find("step.y->gain.u") != std::string::npos);
}