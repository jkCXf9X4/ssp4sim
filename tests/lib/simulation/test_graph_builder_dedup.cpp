// ---------------------------------------------------------------------------
// P5 (FINDING H): duplicate connection-edge dedup in GraphBuilder::wire_connections
//
// Background: the same logical wire is legitimately reachable from BOTH
// endpoints. The source model walks its output connector (conn_children path)
// and the target model walks its input connector (conn_parents path). When the
// analysis graph carries a resolved connection node that is a child of BOTH
// the source output connector AND the target input connector (a shared /
// mirrored-leg topology), each visit constructs an equivalent ConnectionInfo
// and pushes it onto actual_target->connections unconditionally → duplicate
// edges → double execution / double writes at runtime. Model-to-model edges
// are already deduplicated via Node::add_child's contains_child guard; the
// connection adjacency list is a plain vector, so the guard must dedup
// explicitly (see sim_graph_builder.cpp).
//
// These tests build the analysis graph manually with real fixture FMU
// directories (so SspModel/FmuInfo/FmuInstance construction works) and the
// exact shared-connector topology, then run GraphBuilder::build and assert the
// dedup invariant on the produced FmuModel adjacency lists.
// ---------------------------------------------------------------------------
#include "pre/2_analysis_graph/elements/ssp_node.hpp"
#include "pre/3_simulation_graph/builder/sim_graph_builder.hpp"
#include "pre/3_simulation_graph/elements/model_fmu.hpp"
#include "pre/2_analysis_graph/ssp_graph_data.hpp"
#include "utils/config.hpp"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

    using ssp4sim::analysis::ResolvedConnection;
    using ssp4sim::analysis::SspConnector;
    using ssp4sim::analysis::SspConnectorNode;
    using ssp4sim::analysis::SspModel;
    using ssp4sim::analysis::SspModelNode;
    using ssp4sim::analysis::SspNode;

    fs::path project_root()
    {
        return fs::path(SSP4SIM_PROJECT_ROOT);
    }

    fs::path fixture_model_dir(const std::string &model_name)
    {
        return project_root() / "resources" / "reference_ssp" / "artifacts" / "models" /
               "signal_sine_gain_add" / "baseline" / "resources" / model_name;
    }

    /// FmuModel/FmuInfo constructors read global utils::Config (simulation
    /// start/stop/timestep, forward_derivatives, ...). Load the same minimal
    /// JSON used by other simulation tests before building any model.
    void load_minimal_config()
    {
        ssp4sim::utils::Config::loadFromString(R"json(
        {
            "simulation": {
                "start_time": 0.0,
                "stop_time": 1.0,
                "timestep": 0.1,
                "tolerance": 1e-6,
                "executor": { "forward_derivatives": false },
                "log": { "fmu": false }
            }
        }
        )json");
    }

    /// Build an AnalysisGraphData with one wire src.out -> tgt.in, where the
    /// resolved connection node is a child of BOTH the source output connector
    /// and the target input connector (shared-connector / mirrored-leg
    /// topology). This is the topology that makes wire_connections visit the
    /// same wire from both endpoints.
    ///
    /// The models are constructed from real fixture FMU directories so that
    /// SspModel (parses modelDescription.xml) and later FmuInfo/FmuInstance
    /// (native FMU load) succeed.
    /// Owns the analysis graph AND the underlying SspModels so the raw
    /// `source` pointers held by the nodes (SspModelNode -> SspModel,
    /// SspConnectorNode -> SspModel::connectors) stay valid for the full
    /// GraphBuilder::build() call. `data` is declared before `models` so the
    /// node vectors are destroyed before the models they reference.
    struct SharedConnectorGraph
    {
        ssp4sim::analysis::AnalysisGraphData data;
        std::vector<std::unique_ptr<SspModel>> models;
    };

    SharedConnectorGraph build_shared_connector_graph()
    {
        SharedConnectorGraph g;

        // Sine has one output connector "Sine.y"; Gain has input "Gain.u" and
        // output "Gain.y". Wire Sine.y -> Gain.u.
        // Models are heap-owned by `g.models` (vector reallocation only moves
        // the unique_ptrs, never the SspModel objects themselves).
        auto *src_model = g.models.emplace_back(
                              std::make_unique<SspModel>("Sine", fixture_model_dir("Sine").string(),
                                                         std::map<std::string, ssp4sim::ext::ParameterValue>{}))
                              .get();
        auto *tgt_model = g.models.emplace_back(
                              std::make_unique<SspModel>("Gain", fixture_model_dir("Gain").string(),
                                                         std::map<std::string, ssp4sim::ext::ParameterValue>{}))
                              .get();

        auto src_node = std::make_unique<SspModelNode>(src_model);
        auto tgt_node = std::make_unique<SspModelNode>(tgt_model);

        // Find the connectors by name (SspModel::create_connectors names them
        // "<model>.<var>").
        SspConnector *src_conn_src = nullptr;
        SspConnector *tgt_conn_src = nullptr;
        for (auto &c : src_model->connectors)
            if (c.name == "Sine.y") src_conn_src = &c;
        for (auto &c : tgt_model->connectors)
            if (c.name == "Gain.u") tgt_conn_src = &c;
        REQUIRE(src_conn_src != nullptr);
        REQUIRE(tgt_conn_src != nullptr);

        auto src_conn = std::make_unique<SspConnectorNode>(src_conn_src);
        auto tgt_conn = std::make_unique<SspConnectorNode>(tgt_conn_src);

        // Model <-> connector linkage (output connector is a child of the model;
        // input connector is a parent of the model — matches SspGraphBuilder).
        src_node->add_child(src_conn.get());
        tgt_conn->add_child(tgt_node.get());

        // The resolved connection: child of BOTH connectors (shared leg), and
        // each connector is a child of the resolved node (so process_connector
        // finds the peer from either side). This mirrors the real
        // SspGraphBuilder wiring (src_connector->add_child(resolved);
        // resolved->add_child(tgt_connector)) but duplicated on both ends.
        auto resolved = std::make_unique<ResolvedConnection>();
        resolved->name = "wire";
        auto resolved_node = std::make_unique<SspNode<ResolvedConnection>>(resolved.get());
        resolved_node->name = "wire";

        src_conn->add_child(resolved_node.get());
        resolved_node->add_child(tgt_conn.get());
        tgt_conn->add_child(resolved_node.get());
        resolved_node->add_child(src_conn.get());

        g.data.model_nodes.push_back(std::move(src_node));
        g.data.model_nodes.push_back(std::move(tgt_node));
        g.data.connector_nodes.push_back(std::move(src_conn));
        g.data.connector_nodes.push_back(std::move(tgt_conn));
        g.data.connection_sources.push_back(std::move(resolved));
        g.data.connection_nodes.push_back(std::move(resolved_node));

        return g;
    }

    /// Count connections on `model` matching the given wire identity.
    size_t count_wire(
        const ssp4sim::graph::FmuModel *model,
        const ssp4sim::signal::SignalStorage *src_storage,
        uint32_t src_index,
        const ssp4sim::signal::SignalStorage *tgt_storage,
        uint32_t tgt_index)
    {
        size_t count = 0;
        for (const auto &c : model->connections)
        {
            if (c.source_storage == src_storage &&
                c.source_index == src_index &&
                c.target_storage == tgt_storage &&
                c.target_index == tgt_index)
            {
                ++count;
            }
        }
        return count;
    }

} // anonymous namespace

// ---------------------------------------------------------------------------
// Description: A wire shared by both endpoints is appended exactly once
// Rationale:   FINDING H — the shared-connector topology makes wire_connections
//              visit the same wire from the source's conn_children path and the
//              target's conn_parents path. The dedup guard must collapse the
//              two equivalent ConnectionInfo entries into one.
// ---------------------------------------------------------------------------
TEST_CASE("GraphBuilder wires a shared connector exactly once", "[sim_graph_builder][dedup]")
{
    load_minimal_config();
    auto g = build_shared_connector_graph();

    ssp4sim::graph::GraphBuilder builder(false);
    auto models = builder.build(&g.data);

    REQUIRE(models.size() == 2);

    auto *src = dynamic_cast<ssp4sim::graph::FmuModel *>(models.at("Sine").get());
    auto *tgt = dynamic_cast<ssp4sim::graph::FmuModel *>(models.at("Gain").get());
    REQUIRE(src != nullptr);
    REQUIRE(tgt != nullptr);

    // The wire is stored on the target (input side). It must appear exactly once.
    const auto &tgt_in = tgt->inputs.at("Gain.u");
    CHECK(count_wire(tgt, src->output_area.get(), 0, tgt->input_area.get(), tgt_in.index) == 1);
    CHECK(tgt->connections.size() == 1);
}

// ---------------------------------------------------------------------------
// Description: A connection already appended is not duplicated when the same
//              source+target is encountered again via the other route
// Rationale:   Pins the exact FINDING H scenario: same wire identity reached
//              twice ⇒ exactly one entry in the target's connections list.
// ---------------------------------------------------------------------------
TEST_CASE("GraphBuilder dedups a connection seen from both endpoints",
          "[sim_graph_builder][dedup]")
{
    load_minimal_config();
    auto g = build_shared_connector_graph();

    ssp4sim::graph::GraphBuilder builder(false);
    auto models = builder.build(&g.data);

    auto *src = dynamic_cast<ssp4sim::graph::FmuModel *>(models.at("Sine").get());
    auto *tgt = dynamic_cast<ssp4sim::graph::FmuModel *>(models.at("Gain").get());
    REQUIRE(src != nullptr);
    REQUIRE(tgt != nullptr);

    const auto &tgt_in = tgt->inputs.at("Gain.u");
    size_t matches = 0;
    for (const auto &c : tgt->connections)
    {
        if (c.source_storage == src->output_area.get() &&
            c.source_index == 0 &&
            c.target_storage == tgt->input_area.get() &&
            c.target_index == tgt_in.index)
        {
            ++matches;
        }
    }
    CHECK(matches == 1);
}

// ---------------------------------------------------------------------------
// Description: Distinct connections are preserved (no false dedup)
// Rationale:   The guard must compare wire identity (source storage+index,
//              target storage+index), not collapse distinct wires. Two wires
//              into the same target with different source indices must both
//              survive.
// ---------------------------------------------------------------------------
TEST_CASE("GraphBuilder preserves distinct connections", "[sim_graph_builder][dedup]")
{
    load_minimal_config();
    // Two sources (Sine, Step) -> one target (Gain), each with its own resolved
    // leg shared by both endpoints. Both wires are distinct and must both be
    // present exactly once.
    ssp4sim::analysis::AnalysisGraphData data;

    auto src_a = std::make_unique<SspModel>("Sine", fixture_model_dir("Sine").string(),
                                            std::map<std::string, ssp4sim::ext::ParameterValue>{});
    auto src_b = std::make_unique<SspModel>("Step", fixture_model_dir("Step").string(),
                                            std::map<std::string, ssp4sim::ext::ParameterValue>{});
    auto tgt = std::make_unique<SspModel>("Gain", fixture_model_dir("Gain").string(),
                                          std::map<std::string, ssp4sim::ext::ParameterValue>{});

    auto src_a_node = std::make_unique<SspModelNode>(src_a.get());
    auto src_b_node = std::make_unique<SspModelNode>(src_b.get());
    auto tgt_node = std::make_unique<SspModelNode>(tgt.get());

    SspConnector *src_a_conn_src = nullptr;
    SspConnector *src_b_conn_src = nullptr;
    SspConnector *tgt_conn_src = nullptr;
    for (auto &c : src_a->connectors)
        if (c.name == "Sine.y") src_a_conn_src = &c;
    for (auto &c : src_b->connectors)
        if (c.name == "Step.y") src_b_conn_src = &c;
    for (auto &c : tgt->connectors)
        if (c.name == "Gain.u") tgt_conn_src = &c;
    REQUIRE(src_a_conn_src != nullptr);
    REQUIRE(src_b_conn_src != nullptr);
    REQUIRE(tgt_conn_src != nullptr);

    auto src_a_conn = std::make_unique<SspConnectorNode>(src_a_conn_src);
    auto src_b_conn = std::make_unique<SspConnectorNode>(src_b_conn_src);
    auto tgt_conn = std::make_unique<SspConnectorNode>(tgt_conn_src);

    src_a_node->add_child(src_a_conn.get());
    src_b_node->add_child(src_b_conn.get());
    tgt_conn->add_child(tgt_node.get());

    auto resolved_a = std::make_unique<ResolvedConnection>();
    resolved_a->name = "wire_a";
    auto resolved_a_node = std::make_unique<SspNode<ResolvedConnection>>(resolved_a.get());
    resolved_a_node->name = "wire_a";
    src_a_conn->add_child(resolved_a_node.get());
    resolved_a_node->add_child(tgt_conn.get());
    tgt_conn->add_child(resolved_a_node.get());
    resolved_a_node->add_child(src_a_conn.get());

    auto resolved_b = std::make_unique<ResolvedConnection>();
    resolved_b->name = "wire_b";
    auto resolved_b_node = std::make_unique<SspNode<ResolvedConnection>>(resolved_b.get());
    resolved_b_node->name = "wire_b";
    src_b_conn->add_child(resolved_b_node.get());
    resolved_b_node->add_child(tgt_conn.get());
    tgt_conn->add_child(resolved_b_node.get());
    resolved_b_node->add_child(src_b_conn.get());

    data.model_nodes.push_back(std::move(src_a_node));
    data.model_nodes.push_back(std::move(src_b_node));
    data.model_nodes.push_back(std::move(tgt_node));
    data.connector_nodes.push_back(std::move(src_a_conn));
    data.connector_nodes.push_back(std::move(src_b_conn));
    data.connector_nodes.push_back(std::move(tgt_conn));
    data.connection_sources.push_back(std::move(resolved_a));
    data.connection_sources.push_back(std::move(resolved_b));
    data.connection_nodes.push_back(std::move(resolved_a_node));
    data.connection_nodes.push_back(std::move(resolved_b_node));

    ssp4sim::graph::GraphBuilder builder(false);
    auto models = builder.build(&data);

    auto *tgt_model = dynamic_cast<ssp4sim::graph::FmuModel *>(models.at("Gain").get());
    auto *src_a_model = dynamic_cast<ssp4sim::graph::FmuModel *>(models.at("Sine").get());
    auto *src_b_model = dynamic_cast<ssp4sim::graph::FmuModel *>(models.at("Step").get());
    REQUIRE(tgt_model != nullptr);
    REQUIRE(src_a_model != nullptr);
    REQUIRE(src_b_model != nullptr);

    // Both distinct wires must be present exactly once each.
    CHECK(tgt_model->connections.size() == 2);
    CHECK(count_wire(tgt_model, src_a_model->output_area.get(), 0,
                     tgt_model->input_area.get(), tgt_model->inputs.at("Gain.u").index) == 1);
    CHECK(count_wire(tgt_model, src_b_model->output_area.get(), 0,
                     tgt_model->input_area.get(), tgt_model->inputs.at("Gain.u").index) == 1);
}