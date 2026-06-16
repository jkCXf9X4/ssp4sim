#include "analysis/components/analysis_system.hpp"
#include "analysis/analysis_system_builder.hpp"
#include "analysis/components/analysis_model.hpp"
#include "analysis/components/analysis_connector.hpp"

#include "initial_value.hpp"  // StartValue

#include "ssp4cpp/ssp.hpp"
#include "handler/fmu_handler.hpp"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <memory>
#include <set>
#include <string>

namespace fs = std::filesystem;

// Forward declaration of get_start_values (defined in SSP1_SystemStructureParameter_Ext.cpp)
namespace ssp4sim::ext::ssp1::ssv {
    std::vector<StartValue> get_start_values(
        const std::vector<ssp4cpp::ssp1::ssd::ParameterBinding> &bindings,
        const ssp4cpp::Ssp *ssp = nullptr);

    std::map<std::string, StartValue> get_start_value_mappings(
        const std::vector<ssp4cpp::ssp1::ssd::ParameterBinding> &bindings,
        const ssp4cpp::Ssp *ssp = nullptr);
}

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
            if (m->name == name ||
                (m->name.size() > name.size() &&
                 m->name.substr(m->name.size() - name.size() - 1) == "." + name)) {
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

    // Note: With canonical naming, model names are bare local names (e.g., "sine" not "inner.sine").
    // find_model() handles both bare and suffix-matched resolution.

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

// ===========================================================================
// Tests for get_start_values with external SSV/SSM (load_ssv / load_ssm paths)
// ===========================================================================

// ---------------------------------------------------------------------------
// Test Case 7: get_start_values with external SSV (no SSM)
// ---------------------------------------------------------------------------
TEST_CASE("get_start_values with external SSV (no SSM)", "[parameter_binding][get_start_values][external]")
{
    // Use the signal_step_gain fixture which has an external SSV file
    auto fixture_dir = fixture_path("signal_step_gain");
    auto ssp = std::make_unique<ssp4cpp::Ssp>(fixture_dir.string());

    // Construct a ParameterBinding with source pointing to the external SSV
    ssp4cpp::ssp1::ssd::ParameterBinding binding;
    binding.source = "resources/signal_step_gain_parameters.ssv";

    std::vector<ssp4cpp::ssp1::ssd::ParameterBinding> bindings;
    bindings.push_back(std::move(binding));

    auto result = ssp4sim::ext::ssp1::ssv::get_start_values(bindings, ssp.get());

    REQUIRE(result.size() == 4);

    // step.height -> Real, 2.0, default mapping (parameter name itself)
    CHECK(result[0].name == "step.height");
    CHECK(result[0].type == ssp4sim::types::DataType::real);
    CHECK(std::holds_alternative<double>(result[0].value));
    CHECK(std::get<double>(result[0].value) == 2.0);
    REQUIRE(result[0].mappings.size() == 1);
    CHECK(result[0].mappings[0] == "step.height");

    // step.offset -> Real, 1.0, default mapping (parameter name itself)
    CHECK(result[1].name == "step.offset");
    CHECK(result[1].type == ssp4sim::types::DataType::real);
    CHECK(std::holds_alternative<double>(result[1].value));
    CHECK(std::get<double>(result[1].value) == 1.0);
    REQUIRE(result[1].mappings.size() == 1);
    CHECK(result[1].mappings[0] == "step.offset");

    // step.startTime -> Real, 0.25, default mapping (parameter name itself)
    CHECK(result[2].name == "step.startTime");
    CHECK(result[2].type == ssp4sim::types::DataType::real);
    CHECK(std::holds_alternative<double>(result[2].value));
    CHECK(std::get<double>(result[2].value) == 0.25);
    REQUIRE(result[2].mappings.size() == 1);
    CHECK(result[2].mappings[0] == "step.startTime");

    // gain.k -> Real, 3.0, default mapping (parameter name itself)
    CHECK(result[3].name == "gain.k");
    CHECK(result[3].type == ssp4sim::types::DataType::real);
    CHECK(std::holds_alternative<double>(result[3].value));
    CHECK(std::get<double>(result[3].value) == 3.0);
    REQUIRE(result[3].mappings.size() == 1);
    CHECK(result[3].mappings[0] == "gain.k");
}

// ---------------------------------------------------------------------------
// Test Case 8: get_start_values with external SSV and external SSM
// ---------------------------------------------------------------------------
TEST_CASE("get_start_values with external SSV and external SSM", "[parameter_binding][get_start_values][external]")
{
    // Use the signal_step_add fixture which has external SSV + SSM files
    auto fixture_dir = fixture_path("signal_step_add");
    auto ssp = std::make_unique<ssp4cpp::Ssp>(fixture_dir.string());

    // Construct a ParameterBinding with both external SSV source and external SSM source
    ssp4cpp::ssp1::ssd::ParameterBinding binding;
    binding.source = "resources/signal_step_add_parameters.ssv";

    ssp4cpp::ssp1::ssd::ParameterMapping pm;
    pm.source = "resources/signal_step_add_mapping.ssm";
    binding.ParameterMapping = std::move(pm);

    std::vector<ssp4cpp::ssp1::ssd::ParameterBinding> bindings;
    bindings.push_back(std::move(binding));

    auto result = ssp4sim::ext::ssp1::ssv::get_start_values(bindings, ssp.get());

    REQUIRE(result.size() == 8);

    // step_a_height -> Real, 1.5, mapped to step_a.height
    CHECK(result[0].name == "step_a_height");
    CHECK(result[0].type == ssp4sim::types::DataType::real);
    CHECK(std::holds_alternative<double>(result[0].value));
    CHECK(std::get<double>(result[0].value) == 1.5);
    REQUIRE(result[0].mappings.size() == 1);
    CHECK(result[0].mappings[0] == "step_a.height");

    // step_a_offset -> Real, 0.5, mapped to step_a.offset
    CHECK(result[1].name == "step_a_offset");
    CHECK(result[1].type == ssp4sim::types::DataType::real);
    CHECK(std::holds_alternative<double>(result[1].value));
    CHECK(std::get<double>(result[1].value) == 0.5);
    REQUIRE(result[1].mappings.size() == 1);
    CHECK(result[1].mappings[0] == "step_a.offset");

    // step_a_startTime -> Real, 0.25, mapped to step_a.startTime
    CHECK(result[2].name == "step_a_startTime");
    CHECK(result[2].type == ssp4sim::types::DataType::real);
    CHECK(std::holds_alternative<double>(result[2].value));
    CHECK(std::get<double>(result[2].value) == 0.25);
    REQUIRE(result[2].mappings.size() == 1);
    CHECK(result[2].mappings[0] == "step_a.startTime");

    // step_b_height -> Real, -0.5, mapped to step_b.height
    CHECK(result[3].name == "step_b_height");
    CHECK(result[3].type == ssp4sim::types::DataType::real);
    CHECK(std::holds_alternative<double>(result[3].value));
    CHECK(std::get<double>(result[3].value) == -0.5);
    REQUIRE(result[3].mappings.size() == 1);
    CHECK(result[3].mappings[0] == "step_b.height");

    // step_b_offset -> Real, 1.0, mapped to step_b.offset
    CHECK(result[4].name == "step_b_offset");
    CHECK(result[4].type == ssp4sim::types::DataType::real);
    CHECK(std::holds_alternative<double>(result[4].value));
    CHECK(std::get<double>(result[4].value) == 1.0);
    REQUIRE(result[4].mappings.size() == 1);
    CHECK(result[4].mappings[0] == "step_b.offset");

    // step_b_startTime -> Real, 0.5, mapped to step_b.startTime
    CHECK(result[5].name == "step_b_startTime");
    CHECK(result[5].type == ssp4sim::types::DataType::real);
    CHECK(std::holds_alternative<double>(result[5].value));
    CHECK(std::get<double>(result[5].value) == 0.5);
    REQUIRE(result[5].mappings.size() == 1);
    CHECK(result[5].mappings[0] == "step_b.startTime");

    // add_k1 -> Real, 1.0, mapped to add.k1
    CHECK(result[6].name == "add_k1");
    CHECK(result[6].type == ssp4sim::types::DataType::real);
    CHECK(std::holds_alternative<double>(result[6].value));
    CHECK(std::get<double>(result[6].value) == 1.0);
    REQUIRE(result[6].mappings.size() == 1);
    CHECK(result[6].mappings[0] == "add.k1");

    // add_k2 -> Real, 1.0, mapped to add.k2
    CHECK(result[7].name == "add_k2");
    CHECK(result[7].type == ssp4sim::types::DataType::real);
    CHECK(std::holds_alternative<double>(result[7].value));
    CHECK(std::get<double>(result[7].value) == 1.0);
    REQUIRE(result[7].mappings.size() == 1);
    CHECK(result[7].mappings[0] == "add.k2");
}

// ---------------------------------------------------------------------------
// Test Case 9: get_start_value_mappings with external SSV and external SSM
// ---------------------------------------------------------------------------
TEST_CASE("get_start_value_mappings with external SSV and external SSM", "[parameter_binding][get_start_value_mappings][external]")
{
    // Use the signal_step_add fixture which has external SSV + SSM files
    auto fixture_dir = fixture_path("signal_step_add");
    auto ssp = std::make_unique<ssp4cpp::Ssp>(fixture_dir.string());

    // Construct a ParameterBinding with both external SSV source and external SSM source
    ssp4cpp::ssp1::ssd::ParameterBinding binding;
    binding.source = "resources/signal_step_add_parameters.ssv";

    ssp4cpp::ssp1::ssd::ParameterMapping pm;
    pm.source = "resources/signal_step_add_mapping.ssm";
    binding.ParameterMapping = std::move(pm);

    std::vector<ssp4cpp::ssp1::ssd::ParameterBinding> bindings;
    bindings.push_back(std::move(binding));

    auto result = ssp4sim::ext::ssp1::ssv::get_start_value_mappings(bindings, ssp.get());

    // 8 entries in the map, keyed by SSM target names
    REQUIRE(result.size() == 8);

    // step_a.height -> step_a_height, Real, 1.5
    CHECK(result.count("step_a.height") == 1);
    CHECK(result.at("step_a.height").name == "step_a_height");
    CHECK(result.at("step_a.height").type == ssp4sim::types::DataType::real);
    CHECK(std::holds_alternative<double>(result.at("step_a.height").value));
    CHECK(std::get<double>(result.at("step_a.height").value) == 1.5);

    // step_a.offset -> step_a_offset, Real, 0.5
    CHECK(result.count("step_a.offset") == 1);
    CHECK(result.at("step_a.offset").name == "step_a_offset");
    CHECK(result.at("step_a.offset").type == ssp4sim::types::DataType::real);
    CHECK(std::holds_alternative<double>(result.at("step_a.offset").value));
    CHECK(std::get<double>(result.at("step_a.offset").value) == 0.5);

    // step_a.startTime -> step_a_startTime, Real, 0.25
    CHECK(result.count("step_a.startTime") == 1);
    CHECK(result.at("step_a.startTime").name == "step_a_startTime");
    CHECK(result.at("step_a.startTime").type == ssp4sim::types::DataType::real);
    CHECK(std::holds_alternative<double>(result.at("step_a.startTime").value));
    CHECK(std::get<double>(result.at("step_a.startTime").value) == 0.25);

    // step_b.height -> step_b_height, Real, -0.5
    CHECK(result.count("step_b.height") == 1);
    CHECK(result.at("step_b.height").name == "step_b_height");
    CHECK(result.at("step_b.height").type == ssp4sim::types::DataType::real);
    CHECK(std::holds_alternative<double>(result.at("step_b.height").value));
    CHECK(std::get<double>(result.at("step_b.height").value) == -0.5);

    // step_b.offset -> step_b_offset, Real, 1.0
    CHECK(result.count("step_b.offset") == 1);
    CHECK(result.at("step_b.offset").name == "step_b_offset");
    CHECK(result.at("step_b.offset").type == ssp4sim::types::DataType::real);
    CHECK(std::holds_alternative<double>(result.at("step_b.offset").value));
    CHECK(std::get<double>(result.at("step_b.offset").value) == 1.0);

    // step_b.startTime -> step_b_startTime, Real, 0.5
    CHECK(result.count("step_b.startTime") == 1);
    CHECK(result.at("step_b.startTime").name == "step_b_startTime");
    CHECK(result.at("step_b.startTime").type == ssp4sim::types::DataType::real);
    CHECK(std::holds_alternative<double>(result.at("step_b.startTime").value));
    CHECK(std::get<double>(result.at("step_b.startTime").value) == 0.5);

    // add.k1 -> add_k1, Real, 1.0
    CHECK(result.count("add.k1") == 1);
    CHECK(result.at("add.k1").name == "add_k1");
    CHECK(result.at("add.k1").type == ssp4sim::types::DataType::real);
    CHECK(std::holds_alternative<double>(result.at("add.k1").value));
    CHECK(std::get<double>(result.at("add.k1").value) == 1.0);

    // add.k2 -> add_k2, Real, 1.0
    CHECK(result.count("add.k2") == 1);
    CHECK(result.at("add.k2").name == "add_k2");
    CHECK(result.at("add.k2").type == ssp4sim::types::DataType::real);
    CHECK(std::holds_alternative<double>(result.at("add.k2").value));
    CHECK(std::get<double>(result.at("add.k2").value) == 1.0);
}
