#include "analysis/components/analysis_system.hpp"
#include "analysis/analysis_system_builder.hpp"
#include "analysis/components/analysis_model.hpp"
#include "analysis/components/analysis_connector.hpp"

#include "ssp4cpp/ssp.hpp"
#include "handler/fmu_handler.hpp"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <memory>
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

    std::unique_ptr<ssp4sim::analysis::AnalysisSystem> get_sys(const std::string &ssp_path)
    {
        auto ssp = std::make_unique<ssp4cpp::Ssp>(ssp_path);
        auto fmu_handler = std::make_unique<ssp4sim::handler::FmuHandler>(ssp.get());
        fmu_handler->init();
        return ssp4sim::analysis::AnalysisSystemBuilder().build(ssp.get(), fmu_handler.get());
    }

    void check_override(const ssp4sim::analysis::AnalysisModel *model,
                        const std::string &connector_name,
                        double expected_value)
    {
        REQUIRE(model != nullptr);
        for (auto &c : model->connectors) {
            if (c->connector_name == connector_name) {
                REQUIRE(c->initial_value != nullptr);
                CHECK(c->initial_value->type == ssp4sim::types::DataType::real);
                CHECK(std::get<double>(c->initial_value->value) == expected_value);
                return;
            }
        }
        FAIL("Connector " + connector_name + " not found in model " + model->name);
    }

    const ssp4sim::analysis::AnalysisModel *
    find_model(const std::unique_ptr<ssp4sim::analysis::AnalysisSystem> &sys,
               const std::string &name)
    {
        for (auto *m : sys->get_all_models()) {
            if (m->name == name) {
                return m;
            }
        }
        return nullptr;
    }

} // anonymous namespace

// ---------------------------------------------------------------------------
// Test 1: Flat inline-only bindings resolve correctly
// ---------------------------------------------------------------------------
TEST_CASE("Flat inline-only bindings resolve correctly", "[parameter_binding][integration]")
{
    auto sys = get_sys(fixture_path("signal_sine_gain_add").string());

    REQUIRE(sys != nullptr);

    // Verify model names
    std::set<std::string> model_names;
    for (auto *m : sys->get_all_models())
        model_names.insert(m->name);

    CHECK(model_names.count("sine") == 1);
    CHECK(model_names.count("step") == 1);
    CHECK(model_names.count("gain") == 1);
    CHECK(model_names.count("add") == 1);
    CHECK(model_names.size() == 4);

    // sine: amplitude=1.0, f=1.0, offset=0.0, phase=0.0, startTime=0.0
    auto *sine = find_model(sys, "sine");
    check_override(sine, "amplitude", 1.0);
    check_override(sine, "f", 1.0);
    check_override(sine, "offset", 0.0);
    check_override(sine, "phase", 0.0);
    check_override(sine, "startTime", 0.0);

    // step: height=2.0, offset=0.0, startTime=0.5
    auto *step = find_model(sys, "step");
    check_override(step, "height", 2.0);
    check_override(step, "offset", 0.0);
    check_override(step, "startTime", 0.5);

    // gain: k=3.0
    auto *gain = find_model(sys, "gain");
    check_override(gain, "k", 3.0);

    // add: k1=1.0, k2=1.0
    auto *add = find_model(sys, "add");
    check_override(add, "k1", 1.0);
    check_override(add, "k2", 1.0);
}

// ---------------------------------------------------------------------------
// Test 2: Flat external-only bindings resolve correctly
// ---------------------------------------------------------------------------
TEST_CASE("Flat external-only bindings resolve correctly", "[parameter_binding][integration]")
{
    auto sys = get_sys(fixture_path("signal_step_gain").string());

    REQUIRE(sys != nullptr);

    // Verify model names
    std::set<std::string> model_names;
    for (auto *m : sys->get_all_models())
        model_names.insert(m->name);

    CHECK(model_names.count("step") == 1);
    CHECK(model_names.count("gain") == 1);
    CHECK(model_names.size() == 2);

    // step: height=2.0, offset=1.0, startTime=0.25  (from external SSV)
    auto *step = find_model(sys, "step");
    check_override(step, "height", 2.0);
    check_override(step, "offset", 1.0);
    check_override(step, "startTime", 0.25);

    // gain: k=3.0  (from external SSV)
    auto *gain = find_model(sys, "gain");
    check_override(gain, "k", 3.0);
}

// ---------------------------------------------------------------------------
// Test 3: Flat external SSV+SSM bindings resolve correctly
// ---------------------------------------------------------------------------
TEST_CASE("Flat external SSV+SSM bindings resolve correctly", "[parameter_binding][integration]")
{
    auto sys = get_sys(fixture_path("signal_step_add").string());

    REQUIRE(sys != nullptr);

    // Verify model names
    std::set<std::string> model_names;
    for (auto *m : sys->get_all_models())
        model_names.insert(m->name);

    CHECK(model_names.count("step_a") == 1);
    CHECK(model_names.count("step_b") == 1);
    CHECK(model_names.count("add") == 1);
    CHECK(model_names.size() == 3);

    // step_a: height=1.5, offset=0.5, startTime=0.25
    auto *step_a = find_model(sys, "step_a");
    check_override(step_a, "height", 1.5);
    check_override(step_a, "offset", 0.5);
    check_override(step_a, "startTime", 0.25);

    // step_b: height=-0.5, offset=1.0, startTime=0.5
    auto *step_b = find_model(sys, "step_b");
    check_override(step_b, "height", -0.5);
    check_override(step_b, "offset", 1.0);
    check_override(step_b, "startTime", 0.5);

    // add: k1=1.0, k2=1.0
    auto *add = find_model(sys, "add");
    check_override(add, "k1", 1.0);
    check_override(add, "k2", 1.0);
}

// ---------------------------------------------------------------------------
// Test 4: Flat inline SSV with external SSM resolves correctly
// ---------------------------------------------------------------------------
TEST_CASE("Flat inline SSV with external SSM resolves correctly", "[parameter_binding][integration]")
{
    auto sys = get_sys(fixture_path("signal_parameter_inline_with_mapping").string());

    REQUIRE(sys != nullptr);

    // Verify model names
    std::set<std::string> model_names;
    for (auto *m : sys->get_all_models())
        model_names.insert(m->name);

    CHECK(model_names.count("step") == 1);
    CHECK(model_names.size() == 1);

    // step: height=1.0, offset=0.0, startTime=0.25
    auto *step = find_model(sys, "step");
    check_override(step, "height", 1.0);
    check_override(step, "offset", 0.0);
    check_override(step, "startTime", 0.25);
}

// ---------------------------------------------------------------------------
// Test 5: Nested system with root external and nested inline bindings
// ---------------------------------------------------------------------------
TEST_CASE("Nested system with root external and nested inline bindings", "[parameter_binding][integration]")
{
    auto sys = get_sys(fixture_path("signal_nested_parameter_bindings").string());

    REQUIRE(sys != nullptr);

    // Verify nested system structure
    REQUIRE(sys->nested_systems.size() == 1);
    CHECK(sys->nested_systems[0]->name == "inner");

    // Verify all model names
    std::set<std::string> model_names;
    for (auto *m : sys->get_all_models())
        model_names.insert(m->name);

    CHECK(model_names.count("step") == 1);
    CHECK(model_names.count("add") == 1);
    CHECK(model_names.count("sine") == 1);
    CHECK(model_names.count("gain") == 1);
    CHECK(model_names.size() == 4);

    // step: height=1.0, offset=0.0, startTime=0.25 (inline at component level)
    auto *step = find_model(sys, "step");
    check_override(step, "height", 1.0);
    check_override(step, "offset", 0.0);
    check_override(step, "startTime", 0.25);

    // add: k1=1.0, k2=1.0 (from root external SSV+SSM)
    auto *add = find_model(sys, "add");
    check_override(add, "k1", 1.0);
    check_override(add, "k2", 1.0);

    // sine: amplitude=1.0, f=2.0, offset=0.0, phase=0.0, startTime=0.0 (inline at nested system level)
    auto *sine = find_model(sys, "sine");
    check_override(sine, "amplitude", 1.0);
    check_override(sine, "f", 2.0);
    check_override(sine, "offset", 0.0);
    check_override(sine, "phase", 0.0);
    check_override(sine, "startTime", 0.0);

    // gain: k=0.5 (inline at nested component level)
    auto *gain = find_model(sys, "gain");
    check_override(gain, "k", 0.5);
}

// ---------------------------------------------------------------------------
// Test 6: Nested system with external bindings resolves correctly
// ---------------------------------------------------------------------------
TEST_CASE("Nested system with external bindings resolves correctly", "[parameter_binding][integration]")
{
    auto sys = get_sys(fixture_path("signal_nested_external_bindings").string());

    REQUIRE(sys != nullptr);

    // Verify nested system structure
    REQUIRE(sys->nested_systems.size() == 1);
    CHECK(sys->nested_systems[0]->name == "inner");

    // Verify model names
    std::set<std::string> model_names;
    for (auto *m : sys->get_all_models())
        model_names.insert(m->name);

    CHECK(model_names.count("step") == 1);
    CHECK(model_names.count("sine") == 1);
    CHECK(model_names.size() == 2);

    // step: height=1.0, offset=0.0, startTime=0.25 (inline at root component level)
    auto *step = find_model(sys, "step");
    check_override(step, "height", 1.0);
    check_override(step, "offset", 0.0);
    check_override(step, "startTime", 0.25);

    // sine: amplitude=2.0, f=5.0, offset=0.5 (from external SSV+SSM at nested system level)
    auto *sine = find_model(sys, "sine");
    check_override(sine, "amplitude", 2.0);
    check_override(sine, "f", 5.0);
    check_override(sine, "offset", 0.5);
}
