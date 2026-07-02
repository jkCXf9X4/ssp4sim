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

    /// Build an SspSystem from a fixture and trigger parameter application
    /// via SspTreeBuilder. Returns the SspSystem by value; the tree builder
    /// modifies connector initial_values in-place before the copy.
    ssp4sim::analysis::SspSystem get_sys(const std::string &ssp_path)
    {
        // Step 1: construct Ssp
        ssp4cpp::Ssp ssp(ssp_path);

        // Step 2: build SspSystem (applies parameter bindings from SSD/SSV/SSM)
        auto analysis_system = ssp4sim::analysis::SspSystemBuilder().build(&ssp);

        // Step 3: build tree (applies parameters to connector initial_values)
        ssp4sim::analysis::SspTreeBuilder tree_builder;
        tree_builder.build(&analysis_system);

        return analysis_system;
    }

    /// Recursively find a model by name in the SspSystem hierarchy.
    ssp4sim::analysis::SspModel *find_model(
        ssp4sim::analysis::SspSystem &sys,
        const std::string &name)
    {
        for (auto &model : sys.models)
        {
            if (model.name == name)
                return &model;
        }
        for (auto &nested : sys.nested_systems)
        {
            if (auto *m = find_model(nested, name))
                return m;
        }
        return nullptr;
    }

    /// Check that a connector in a model has the expected parameter override.
    /// Connector names are in "model.variable" format (e.g. "sine.amplitude").
    void check_override(const ssp4sim::analysis::SspModel *model,
                        const std::string &connector_name,
                        double expected_value)
    {
        REQUIRE(model != nullptr);
        for (const auto &c : model->connectors)
        {
            if (c.name == connector_name)
            {
                CHECK(c.initial_value.type == ssp4sim::types::DataType::real);
                CHECK(std::holds_alternative<double>(c.initial_value.value));
                CHECK(std::get<double>(c.initial_value.value) == expected_value);
                return;
            }
        }
        FAIL("Connector " << connector_name << " not found in model " << model->name);
    }

} // anonymous namespace

// ---------------------------------------------------------------------------
// Description: Full pipeline with inline SSV (sine_gain_add fixture)
// Rationale:   End-to-end parameter binding with inline SSV data
// ---------------------------------------------------------------------------
TEST_CASE("Flat inline-only bindings resolve correctly", "[parameter_binding][integration]")
{
    auto sys = get_sys(fixture_path("signal_sine_gain_add").string());

    // Verify model names
    std::set<std::string> model_names;
    for (const auto &m : sys.models)
        model_names.insert(m.name);

    CHECK(model_names.count("sine") == 1);
    CHECK(model_names.count("step") == 1);
    CHECK(model_names.count("gain") == 1);
    CHECK(model_names.count("add") == 1);
    CHECK(model_names.size() == 4);

    // sine: amplitude=1.0, f=1.0, offset=0.0, phase=0.0, startTime=0.0
    auto *sine = find_model(sys, "sine");
    check_override(sine, "sine.amplitude", 1.0);
    check_override(sine, "sine.f", 1.0);
    check_override(sine, "sine.offset", 0.0);
    check_override(sine, "sine.phase", 0.0);
    check_override(sine, "sine.startTime", 0.0);

    // step: height=2.0, offset=0.0, startTime=0.5
    auto *step = find_model(sys, "step");
    check_override(step, "step.height", 2.0);
    check_override(step, "step.offset", 0.0);
    check_override(step, "step.startTime", 0.5);

    // gain: k=3.0
    auto *gain = find_model(sys, "gain");
    check_override(gain, "gain.k", 3.0);

    // add: k1=1.0, k2=1.0
    auto *add = find_model(sys, "add");
    check_override(add, "add.k1", 1.0);
    check_override(add, "add.k2", 1.0);
}

// ---------------------------------------------------------------------------
// Description: Full pipeline with external SSV (step_gain fixture)
// Rationale:   End-to-end with external SSV
// ---------------------------------------------------------------------------
TEST_CASE("Flat external-only bindings resolve correctly", "[parameter_binding][integration]")
{
    auto sys = get_sys(fixture_path("signal_step_gain").string());

    // Verify model names
    std::set<std::string> model_names;
    for (const auto &m : sys.models)
        model_names.insert(m.name);

    CHECK(model_names.count("step") == 1);
    CHECK(model_names.count("gain") == 1);
    CHECK(model_names.size() == 2);

    // step: height=2.0, offset=1.0, startTime=0.25  (from external SSV)
    auto *step = find_model(sys, "step");
    check_override(step, "step.height", 2.0);
    check_override(step, "step.offset", 1.0);
    check_override(step, "step.startTime", 0.25);

    // gain: k=3.0  (from external SSV)
    auto *gain = find_model(sys, "gain");
    check_override(gain, "gain.k", 3.0);
}

// ---------------------------------------------------------------------------
// Description: Full pipeline with external SSV+SSM (step_add fixture)
// Rationale:   End-to-end with SSM remapping
// ---------------------------------------------------------------------------
TEST_CASE("Flat external SSV+SSM bindings resolve correctly", "[parameter_binding][integration]")
{
    auto sys = get_sys(fixture_path("signal_step_add").string());

    // Verify model names
    std::set<std::string> model_names;
    for (const auto &m : sys.models)
        model_names.insert(m.name);

    CHECK(model_names.count("step_a") == 1);
    CHECK(model_names.count("step_b") == 1);
    CHECK(model_names.count("add") == 1);
    CHECK(model_names.size() == 3);

    // step_a: height=1.5, offset=0.5, startTime=0.25
    auto *step_a = find_model(sys, "step_a");
    check_override(step_a, "step_a.height", 1.5);
    check_override(step_a, "step_a.offset", 0.5);
    check_override(step_a, "step_a.startTime", 0.25);

    // step_b: height=-0.5, offset=1.0, startTime=0.5
    auto *step_b = find_model(sys, "step_b");
    check_override(step_b, "step_b.height", -0.5);
    check_override(step_b, "step_b.offset", 1.0);
    check_override(step_b, "step_b.startTime", 0.5);

    // add: k1=1.0, k2=1.0
    auto *add = find_model(sys, "add");
    check_override(add, "add.k1", 1.0);
    check_override(add, "add.k2", 1.0);
}

// ---------------------------------------------------------------------------
// Description: Full pipeline with inline SSV + external SSM
// Rationale:   Combinatorial coverage
// ---------------------------------------------------------------------------
TEST_CASE("Flat inline SSV with external SSM resolves correctly", "[parameter_binding][integration]")
{
    auto sys = get_sys(fixture_path("signal_parameter_inline_with_mapping").string());

    // Verify model names
    std::set<std::string> model_names;
    for (const auto &m : sys.models)
        model_names.insert(m.name);

    CHECK(model_names.count("step") == 1);
    CHECK(model_names.size() == 1);

    // step: height=1.0, offset=0.0, startTime=0.25
    auto *step = find_model(sys, "step");
    check_override(step, "step.height", 1.0);
    check_override(step, "step.offset", 0.0);
    check_override(step, "step.startTime", 0.25);
}

// ---------------------------------------------------------------------------
// Description: Nested SSP with bindings at multiple levels
// Rationale:   Nested parameter resolution is the most complex scenario
// ---------------------------------------------------------------------------
TEST_CASE("Nested system with root external and nested inline bindings", "[parameter_binding][integration]")
{
    auto sys = get_sys(fixture_path("signal_nested_parameter_bindings").string());

    // Verify nested system structure
    REQUIRE(sys.nested_systems.size() == 1);
    CHECK(sys.nested_systems[0].name == "inner");

    // Verify all model names
    std::set<std::string> model_names;
    for (const auto &m : sys.models)
        model_names.insert(m.name);
    for (const auto &nested : sys.nested_systems)
        for (const auto &m : nested.models)
            model_names.insert(m.name);

    CHECK(model_names.count("step") == 1);
    CHECK(model_names.count("add") == 1);
    CHECK(model_names.count("sine") == 1);
    CHECK(model_names.count("gain") == 1);
    CHECK(model_names.size() == 4);

    // step: height=1.0, offset=0.0, startTime=0.25 (inline at component level)
    auto *step = find_model(sys, "step");
    check_override(step, "step.height", 1.0);
    check_override(step, "step.offset", 0.0);
    check_override(step, "step.startTime", 0.25);

    // add: k1=1.0, k2=1.0 (from root external SSV+SSM)
    auto *add = find_model(sys, "add");
    check_override(add, "add.k1", 1.0);
    check_override(add, "add.k2", 1.0);

    // sine: amplitude=1.0, f=2.0, offset=0.0, phase=0.0, startTime=0.0 (inline at nested system level)
    auto *sine = find_model(sys, "sine");
    check_override(sine, "sine.amplitude", 1.0);
    check_override(sine, "sine.f", 2.0);
    check_override(sine, "sine.offset", 0.0);
    check_override(sine, "sine.phase", 0.0);
    check_override(sine, "sine.startTime", 0.0);

    // gain: k=0.5 (inline at nested component level)
    auto *gain = find_model(sys, "gain");
    check_override(gain, "gain.k", 0.5);
}

// ---------------------------------------------------------------------------
// Description: Nested SSP with external bindings at nested level
// Rationale:   Coverage of external bindings in nested context
// ---------------------------------------------------------------------------
TEST_CASE("Nested system with external bindings resolves correctly", "[parameter_binding][integration]")
{
    auto sys = get_sys(fixture_path("signal_nested_external_bindings").string());

    // Verify nested system structure
    REQUIRE(sys.nested_systems.size() == 1);
    CHECK(sys.nested_systems[0].name == "inner");

    // Verify model names
    std::set<std::string> model_names;
    for (const auto &m : sys.models)
        model_names.insert(m.name);
    for (const auto &nested : sys.nested_systems)
        for (const auto &m : nested.models)
            model_names.insert(m.name);

    CHECK(model_names.count("step") == 1);
    CHECK(model_names.count("sine") == 1);
    CHECK(model_names.size() == 2);

    // step: height=1.0, offset=0.0, startTime=0.25 (inline at root component level)
    auto *step = find_model(sys, "step");
    check_override(step, "step.height", 1.0);
    check_override(step, "step.offset", 0.0);
    check_override(step, "step.startTime", 0.25);

    // sine: amplitude=2.0, f=5.0, offset=0.5 (from external SSV+SSM at nested system level)
    auto *sine = find_model(sys, "sine");
    check_override(sine, "sine.amplitude", 2.0);
    check_override(sine, "sine.f", 5.0);
    check_override(sine, "sine.offset", 0.5);
}