

#include "SSP1_SystemStructureParameter_Ext.hpp"   // get_start_values, get_start_value_map
#include "initial_value.hpp"                        // StartValue
#include "ssp4cpp/ssp.hpp"                         // ParameterBindings, Ssp

#include <catch2/catch_test_macros.hpp>

#include <map>
#include <string>
#include <vector>

using ssp4sim::ext::ssp1::ssv::StartValue;
using ssp4sim::types::DataType;

// External vs inline ParameterSets and ParameterMappings
// -------------------------------------------------------
// The SSP XML schema allows SSV (ParameterSet) and SSM (ParameterMapping)
// data to be either:
//   a) inline (embedded directly in the SSD XML element), or
//   b) external (referenced via a 'source' URI attribute pointing to a
//      separate .ssv or .ssm file).
//
// Resolution of inline vs external happens in ssp4cpp/ssp.cpp
// (get_parameter_set and get_parameter_mapping). Both paths produce
// identical C++ types (ssp4cpp::ssp1::ssv::ParameterSet and
// ssp4cpp::ssp1::ssm::ParameterMapping). The API functions tested in
// this file (get_start_values, get_start_value_map) operate on these
// already-resolved types and are therefore source-agnostic.
//
// The cases below exercise all four combinatorial patterns explicitly
// to document coverage, even though the data-processing code paths
// are identical for inline and externally-sourced data. The XML
// parsing resolution itself is integration-tested in ssp4cpp's
// test_import.cpp using real SSP archives.
// ---------------------------------------------------------------------------
// Helper: construct a TParameter with a real value
// ---------------------------------------------------------------------------
static ssp4cpp::ssp1::ssv::TParameter make_real_param(const std::string &name, double value)
{
    ssp4cpp::ssp1::ssv::TParameter p;
    p.name = name;
    ssp4cpp::ssp1::ssv::Real r;
    r.value = value;
    p.Real = r;
    return p;
}

// Helper: construct a TParameter with an integer value
static ssp4cpp::ssp1::ssv::TParameter make_int_param(const std::string &name, int value)
{
    ssp4cpp::ssp1::ssv::TParameter p;
    p.name = name;
    ssp4cpp::ssp1::ssv::Integer i;
    i.value = value;
    p.Integer = i;
    return p;
}

// Helper: construct a TParameter with a string value
static ssp4cpp::ssp1::ssv::TParameter make_string_param(const std::string &name, const std::string &value)
{
    ssp4cpp::ssp1::ssv::TParameter p;
    p.name = name;
    ssp4cpp::ssp1::ssv::String s;
    s.value = value;
    p.String = s;
    return p;
}

// ---------------------------------------------------------------------------
// Test Case 1: System-level SSV-only bindings
// ---------------------------------------------------------------------------
TEST_CASE("System-level SSV-only bindings with mixed types", "[parameter_binding]")
{
    // Create TParameters with mixed-type parameters
    ssp4cpp::ssp1::ssv::TParameters params;
    params.Parameters = {
        make_real_param("gain.k", 3.0),
        make_int_param("step.count", 5),
        make_string_param("step.mode", "linear")
    };

    // Create ParameterSet
    ssp4cpp::ssp1::ssv::ParameterSet param_set;
    param_set.name = "test_set";
    param_set.Parameters = std::move(params);

    // Create ParameterBindings (no SSM)
    ssp4cpp::ParameterBindings bindings;
    bindings.ssv = std::move(param_set);

    std::vector<ssp4cpp::ParameterBindings> bindings_vec;
    bindings_vec.push_back(std::move(bindings));

    auto start_values = ssp4sim::ext::ssp1::ssv::get_start_values(bindings_vec);

    REQUIRE(start_values.size() == 3);

    // --- gain.k : real = 3.0 ---
    CHECK(start_values[0].name == "gain.k");
    CHECK(start_values[0].type == DataType::real);
    CHECK(std::holds_alternative<double>(start_values[0].value));
    CHECK(std::get<double>(start_values[0].value) == 3.0);
    CHECK(start_values[0].mappings.size() == 1);
    CHECK(start_values[0].mappings[0] == "gain.k");

    // --- step.count : integer = 5 ---
    CHECK(start_values[1].name == "step.count");
    CHECK(start_values[1].type == DataType::integer);
    CHECK(std::holds_alternative<int>(start_values[1].value));
    CHECK(std::get<int>(start_values[1].value) == 5);
    CHECK(start_values[1].mappings.size() == 1);
    CHECK(start_values[1].mappings[0] == "step.count");

    // --- step.mode : string = "linear" ---
    CHECK(start_values[2].name == "step.mode");
    CHECK(start_values[2].type == DataType::string);
    CHECK(std::holds_alternative<std::string>(start_values[2].value));
    CHECK(std::get<std::string>(start_values[2].value) == "linear");
    CHECK(start_values[2].mappings.size() == 1);
    CHECK(start_values[2].mappings[0] == "step.mode");
}

// ---------------------------------------------------------------------------
// Test Case 2: System-level bindings with SSV and SSM
// ---------------------------------------------------------------------------
TEST_CASE("System-level bindings with SSV and SSM parameter mapping", "[parameter_binding]")
{
    // Create ParameterSet with one parameter "k" = 2.0 (real)
    ssp4cpp::ssp1::ssv::TParameters params;
    params.Parameters = { make_real_param("k", 2.0) };

    ssp4cpp::ssp1::ssv::ParameterSet param_set;
    param_set.name = "test";
    param_set.Parameters = std::move(params);

    // Create TMappingEntry
    ssp4cpp::ssp1::ssm::TMappingEntry entry;
    entry.source = "k";
    entry.target = "fmu.k";

    // Create ParameterMapping
    ssp4cpp::ssp1::ssm::ParameterMapping mapping;
    mapping.version = "1.0";
    mapping.MappingEntry = {entry};

    // Create ParameterBindings with both SSV and SSM
    ssp4cpp::ParameterBindings bindings;
    bindings.ssv = std::move(param_set);
    bindings.ssm = std::move(mapping);

    std::vector<ssp4cpp::ParameterBindings> bindings_vec;
    bindings_vec.push_back(std::move(bindings));

    auto start_values = ssp4sim::ext::ssp1::ssv::get_start_values(bindings_vec);

    REQUIRE(start_values.size() == 1);
    CHECK(start_values[0].name == "k");
    CHECK(start_values[0].mappings.size() == 2);
    CHECK(start_values[0].mappings[0] == "k");
    CHECK(start_values[0].mappings[1] == "fmu.k");
    CHECK(std::holds_alternative<double>(start_values[0].value));
    CHECK(std::get<double>(start_values[0].value) == 2.0);
}

// ---------------------------------------------------------------------------
// Test Case 3: Component-level bindings (prepended component prefix)
// ---------------------------------------------------------------------------
TEST_CASE("Component-level parameter bindings with name prefix", "[parameter_binding]")
{
    // Simulate a component-level binding where the parameter name
    // already includes the component prefix
    ssp4cpp::ssp1::ssv::TParameters params;
    params.Parameters = { make_real_param("gain.k", 3.0) };

    ssp4cpp::ssp1::ssv::ParameterSet param_set;
    param_set.name = "component_binding";
    param_set.Parameters = std::move(params);

    ssp4cpp::ParameterBindings bindings;
    bindings.ssv = std::move(param_set);

    std::vector<ssp4cpp::ParameterBindings> bindings_vec;
    bindings_vec.push_back(std::move(bindings));

    auto start_values = ssp4sim::ext::ssp1::ssv::get_start_values(bindings_vec);

    REQUIRE(start_values.size() == 1);
    CHECK(start_values[0].name == "gain.k");
    CHECK(std::holds_alternative<double>(start_values[0].value));
    CHECK(std::get<double>(start_values[0].value) == 3.0);
    CHECK(start_values[0].mappings.size() == 1);
    CHECK(start_values[0].mappings[0] == "gain.k");
}

// ---------------------------------------------------------------------------
// Test Case 4: Empty bindings
// ---------------------------------------------------------------------------
TEST_CASE("Empty bindings produce empty start values", "[parameter_binding]")
{
    std::vector<ssp4cpp::ParameterBindings> empty;
    auto result = ssp4sim::ext::ssp1::ssv::get_start_values(empty);
    REQUIRE(result.empty());
}

// ---------------------------------------------------------------------------
// Test Case 5: get_start_value_map
// ---------------------------------------------------------------------------
TEST_CASE("get_start_value_map assembles name-to-StartValue map", "[parameter_binding]")
{
    // Manually construct a StartValue with two mappings
    StartValue sv("k", DataType::real);
    sv.mappings.push_back("fmu.k");
    double val = 2.0;
    sv.store_value(&val);

    std::vector<StartValue> values = {sv};
    auto result = ssp4sim::ext::ssp1::ssv::get_start_value_map(values);

    REQUIRE(result.size() == 2);

    {
        auto it = result.find("k");
        REQUIRE(it != result.end());
        CHECK(it->second.name == "k");
        CHECK(it->second.type == DataType::real);
        CHECK(std::get<double>(it->second.value) == 2.0);
    }
    {
        auto it = result.find("fmu.k");
        REQUIRE(it != result.end());
        CHECK(it->second.name == "k");
        CHECK(it->second.type == DataType::real);
        CHECK(std::get<double>(it->second.value) == 2.0);
    }
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// Test Case 7: External ParameterSet (constructed), no ParameterMapping
// ---------------------------------------------------------------------------
TEST_CASE("External ParameterSet (constructed), no ParameterMapping", "[parameter_binding]")
{
    // Create TParameters with two real parameters (simulating external SSV)
    ssp4cpp::ssp1::ssv::TParameters params;
    params.Parameters = {
        make_real_param("ext.k", 4.0),
        make_real_param("ext.gain", 1.5)
    };

    // Create ParameterSet
    ssp4cpp::ssp1::ssv::ParameterSet param_set;
    param_set.name = "external_ssv";
    param_set.Parameters = std::move(params);

    // Create ParameterBindings with SSV and no SSM (simulating external SSV source)
    ssp4cpp::ParameterBindings bindings;
    bindings.ssv = std::move(param_set);

    std::vector<ssp4cpp::ParameterBindings> bindings_vec;
    bindings_vec.push_back(std::move(bindings));

    auto start_values = ssp4sim::ext::ssp1::ssv::get_start_values(bindings_vec);

    REQUIRE(start_values.size() == 2);
    CHECK(start_values[0].name == "ext.k");
    CHECK(std::holds_alternative<double>(start_values[0].value));
    CHECK(std::get<double>(start_values[0].value) == 4.0);
    CHECK(start_values[0].type == DataType::real);
    CHECK(start_values[0].mappings.size() == 1);
    CHECK(start_values[0].mappings[0] == "ext.k");

    CHECK(start_values[1].name == "ext.gain");
    CHECK(std::holds_alternative<double>(start_values[1].value));
    CHECK(std::get<double>(start_values[1].value) == 1.5);
    CHECK(start_values[1].type == DataType::real);
    CHECK(start_values[1].mappings.size() == 1);
    CHECK(start_values[1].mappings[0] == "ext.gain");
}

// ---------------------------------------------------------------------------
// Test Case 8: Inline ParameterSet with external ParameterMapping (constructed)
// ---------------------------------------------------------------------------
TEST_CASE("Inline ParameterSet with external ParameterMapping (constructed)", "[parameter_binding]")
{
    // Create ParameterSet with one parameter (simulating inline SSV)
    ssp4cpp::ssp1::ssv::TParameters params;
    params.Parameters = { make_real_param("a", 1.0) };

    ssp4cpp::ssp1::ssv::ParameterSet param_set;
    param_set.name = "inline_ssv";
    param_set.Parameters = std::move(params);

    // Create TMappingEntry (simulating external SSM)
    ssp4cpp::ssp1::ssm::TMappingEntry entry;
    entry.source = "a";
    entry.target = "sink.a";

    // Create ParameterMapping
    ssp4cpp::ssp1::ssm::ParameterMapping mapping;
    mapping.version = "1.0";
    mapping.MappingEntry = {entry};

    // Create ParameterBindings with both (simulating inline SSV + external SSM)
    ssp4cpp::ParameterBindings bindings;
    bindings.ssv = std::move(param_set);
    bindings.ssm = std::move(mapping);

    std::vector<ssp4cpp::ParameterBindings> bindings_vec;
    bindings_vec.push_back(std::move(bindings));

    auto start_values = ssp4sim::ext::ssp1::ssv::get_start_values(bindings_vec);

    REQUIRE(start_values.size() == 1);
    CHECK(start_values[0].name == "a");
    CHECK(start_values[0].mappings.size() == 2);
    CHECK(start_values[0].mappings[0] == "a");
    CHECK(start_values[0].mappings[1] == "sink.a");
}

// ---------------------------------------------------------------------------
// Test Case 9: External ParameterSet with external ParameterMapping (constructed)
// ---------------------------------------------------------------------------
TEST_CASE("External ParameterSet with external ParameterMapping (constructed)", "[parameter_binding]")
{
    // Create ParameterSet with one parameter (simulating external SSV)
    ssp4cpp::ssp1::ssv::TParameters params;
    params.Parameters = { make_real_param("b", 9.0) };

    ssp4cpp::ssp1::ssv::ParameterSet param_set;
    param_set.name = "external_both";
    param_set.Parameters = std::move(params);

    // Create TMappingEntry (simulating external SSM)
    ssp4cpp::ssp1::ssm::TMappingEntry entry;
    entry.source = "b";
    entry.target = "out.b";

    // Create ParameterMapping
    ssp4cpp::ssp1::ssm::ParameterMapping mapping;
    mapping.version = "1.0";
    mapping.MappingEntry = {entry};

    // Create ParameterBindings with both (simulating all-external scenario)
    ssp4cpp::ParameterBindings bindings;
    bindings.ssv = std::move(param_set);
    bindings.ssm = std::move(mapping);

    std::vector<ssp4cpp::ParameterBindings> bindings_vec;
    bindings_vec.push_back(std::move(bindings));

    auto start_values = ssp4sim::ext::ssp1::ssv::get_start_values(bindings_vec);

    REQUIRE(start_values.size() == 1);
    CHECK(start_values[0].name == "b");
    CHECK(std::holds_alternative<double>(start_values[0].value));
    CHECK(std::get<double>(start_values[0].value) == 9.0);
    CHECK(start_values[0].mappings.size() == 2);
    CHECK(start_values[0].mappings[0] == "b");
    CHECK(start_values[0].mappings[1] == "out.b");
}

// ---------------------------------------------------------------------------
// Test Case 10: Empty external ParameterSet edge case
// ---------------------------------------------------------------------------
TEST_CASE("Empty external ParameterSet edge case", "[parameter_binding]")
{
    // Create empty TParameters (simulating external SSV with no parameters)
    ssp4cpp::ssp1::ssv::TParameters params;

    // Create ParameterSet with empty TParameters
    ssp4cpp::ssp1::ssv::ParameterSet param_set;
    param_set.name = "empty_ssv";
    param_set.Parameters = std::move(params);

    // Create ParameterBindings with empty SSV and no SSM
    ssp4cpp::ParameterBindings bindings;
    bindings.ssv = std::move(param_set);

    std::vector<ssp4cpp::ParameterBindings> bindings_vec;
    bindings_vec.push_back(std::move(bindings));

    auto start_values = ssp4sim::ext::ssp1::ssv::get_start_values(bindings_vec);

    REQUIRE(start_values.size() == 0);
    CHECK(start_values.empty());
}

// ---------------------------------------------------------------------------
// Test Case 11: Real SSP fixture resolves external SSV/SSM (integration)
// ---------------------------------------------------------------------------
TEST_CASE("Real SSP fixture resolves external SSV/SSM", "[parameter_binding][integration]")
{
    auto ssp_path = std::filesystem::path(SSP4SIM_PROJECT_ROOT)
        / "resources" / "reference_ssp" / "artifacts" / "models"
        / "signal_nested_parameter_bindings" / "baseline";
    REQUIRE(std::filesystem::exists(ssp_path / "SystemStructure.ssd"));

    ssp4cpp::Ssp ssp(ssp_path);

    // Root system: external SSV + SSM (add_k1=1.0, add_k2=1.0 mapped to add.k1, add.k2)
    // + component step inline (height=1.0, offset=0.0, startTime=0.25)
    REQUIRE(ssp.parameter_bindings.size() >= 2);

    // Binding[0]: root system external SSV + SSM
    {
        auto &bind = ssp.parameter_bindings[0];

        // External SSV: add_k1=1.0, add_k2=1.0
        REQUIRE(bind.ssv.Parameters.Parameters.size() == 2);
        CHECK(bind.ssv.Parameters.Parameters[0].name == "add_k1");
        CHECK(bind.ssv.Parameters.Parameters[0].Real->value == 1.0);
        CHECK(bind.ssv.Parameters.Parameters[1].name == "add_k2");
        CHECK(bind.ssv.Parameters.Parameters[1].Real->value == 1.0);

        // External SSM: add_k1 to add.k1, add_k2 to add.k2
        REQUIRE(bind.ssm.has_value());
        REQUIRE(bind.ssm->MappingEntry.size() == 2);
        CHECK(bind.ssm->MappingEntry[0].source == "add_k1");
        CHECK(bind.ssm->MappingEntry[0].target == "add.k1");
        CHECK(bind.ssm->MappingEntry[1].source == "add_k2");
        CHECK(bind.ssm->MappingEntry[1].target == "add.k2");
    }

    // Binding[1]: component step inline parameter set
    {
        auto &bind = ssp.parameter_bindings[1];
        REQUIRE(bind.ssv.Parameters.Parameters.size() == 3);

        // Find parameters by component-prefixed name (ssp4cpp prepends component name)
        auto find_param = [&](const std::string &name) -> const ssp4cpp::ssp1::ssv::TParameter* {
            for (auto &p : bind.ssv.Parameters.Parameters)
                if (p.name == name) return &p;
            return nullptr;
        };

        const auto *height = find_param("step.height");
        REQUIRE(height != nullptr);
        CHECK(height->Real->value == 1.0);

        const auto *offset = find_param("step.offset");
        REQUIRE(offset != nullptr);
        CHECK(offset->Real->value == 0.0);

        const auto *startTime = find_param("step.startTime");
        REQUIRE(startTime != nullptr);
        CHECK(startTime->Real->value == 0.25);
    }

    // Verify the assembled start-value map via get_start_value_mappings
    auto start_map = ssp4sim::ext::ssp1::ssv::get_start_value_mappings(ssp);

    // Source names from SSV
    CHECK(start_map.contains("add_k1"));
    CHECK(start_map.contains("add_k2"));

    // Mapped target names from SSM
    CHECK(start_map.contains("add.k1"));
    CHECK(start_map.contains("add.k2"));

    // Component-level prefixed names
    CHECK(start_map.contains("step.height"));
    CHECK(start_map.contains("step.offset"));
    CHECK(start_map.contains("step.startTime"));

    // Verify types and values
    auto check_real = [&](const std::string &name, double expected) {
        auto it = start_map.find(name);
        REQUIRE(it != start_map.end());
        CHECK(it->second.type == ssp4sim::types::DataType::real);
        CHECK(std::get<double>(it->second.value) == expected);
    };
    check_real("add_k1", 1.0);
    check_real("add.k1", 1.0);
    check_real("add_k2", 1.0);
    check_real("add.k2", 1.0);
    check_real("step.height", 1.0);
    check_real("step.offset", 0.0);
    check_real("step.startTime", 0.25);

    // Known limitation: nested system inner's bindings (sine.*, gain.k)
    // are not traversed by ssp4cpp's get_parameter_bindings()
    WARN("Nested system parameter bindings are not traversed by ssp4cpp (known limitation). "
         "This fixture has additional inline bindings inside the 'inner' nested system "
         "that are not captured in ssp.parameter_bindings.");
}
