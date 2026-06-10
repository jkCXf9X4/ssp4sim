

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
// Test Case 6: Nested sub-system parameter bindings (SKIP / xfail)
// ---------------------------------------------------------------------------
TEST_CASE("Nested sub-system parameter bindings are not traversed", "[parameter_binding]")
{
    SKIP("Nested system parameter bindings are not yet supported -- "
         "get_parameter_bindings() in ssp4cpp/ssp.cpp only "
         "traverses root System.ParameterBindings and "
         "root System.Elements.Components[*].ParameterBindings. "
         "Nested System elements inside Elements are skipped.");

    // If executed, this test would:
    // auto ssp = ssp4cpp::Ssp(SSP4SIM_PROJECT_ROOT "/resources/reference_ssp/models/ssp/dcmotor/ssp/");
    // auto param_count = ssp.parameter_bindings.size();
    // ... but the traversal code at ssp.cpp:62-117 only reads
    // ssd.System.Elements.Components, never ssd.System.Elements.Systems,
    // so nested bindings are not captured.
}

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
