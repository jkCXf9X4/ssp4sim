#include "SSP1_SystemStructureParameter_Ext.hpp"   // get_start_value_mappings
#include "initial_value.hpp"                        // StartValue

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
// this file (get_start_value_mappings) operate on these already-resolved
// types and are therefore source-agnostic.
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

    // Create ParameterBinding (SSV only)
    ssp4cpp::ssp1::ssd::ParameterBinding binding;
    ssp4cpp::ssp1::ssd::ParameterValues pv;
    pv.ParameterSet = std::move(param_set);
    binding.ParameterValues = std::move(pv);

    std::vector<ssp4cpp::ssp1::ssd::ParameterBinding> bindings_vec;
    bindings_vec.push_back(std::move(binding));

    auto result = ssp4sim::ext::ssp1::ssv::get_start_value_mappings(bindings_vec);

    REQUIRE(result.size() == 3);

    // --- gain.k : real = 3.0 ---
    {
        auto it = result.find("gain.k");
        REQUIRE(it != result.end());
        CHECK(it->second.name == "gain.k");
        CHECK(it->second.type == DataType::real);
        CHECK(std::holds_alternative<double>(it->second.value));
        CHECK(std::get<double>(it->second.value) == 3.0);
        CHECK(it->second.mappings.size() == 1);
        CHECK(it->second.mappings[0] == "gain.k");
    }

    // --- step.count : integer = 5 ---
    {
        auto it = result.find("step.count");
        REQUIRE(it != result.end());
        CHECK(it->second.name == "step.count");
        CHECK(it->second.type == DataType::integer);
        CHECK(std::holds_alternative<int>(it->second.value));
        CHECK(std::get<int>(it->second.value) == 5);
        CHECK(it->second.mappings.size() == 1);
        CHECK(it->second.mappings[0] == "step.count");
    }

    // --- step.mode : string = "linear" ---
    {
        auto it = result.find("step.mode");
        REQUIRE(it != result.end());
        CHECK(it->second.name == "step.mode");
        CHECK(it->second.type == DataType::string);
        CHECK(std::holds_alternative<std::string>(it->second.value));
        CHECK(std::get<std::string>(it->second.value) == "linear");
        CHECK(it->second.mappings.size() == 1);
        CHECK(it->second.mappings[0] == "step.mode");
    }
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
    ssp4cpp::ssp1::ssm::ParameterMapping ssm_mapping;
    ssm_mapping.version = "1.0";
    ssm_mapping.MappingEntry = {entry};

    // Create ParameterBinding with both SSV and SSM
    ssp4cpp::ssp1::ssd::ParameterBinding binding;
    ssp4cpp::ssp1::ssd::ParameterValues pv;
    pv.ParameterSet = std::move(param_set);
    binding.ParameterValues = std::move(pv);
    ssp4cpp::ssp1::ssd::ParameterMapping pm;
    pm.ParameterMapping = std::move(ssm_mapping);
    binding.ParameterMapping = std::move(pm);

    std::vector<ssp4cpp::ssp1::ssd::ParameterBinding> bindings_vec;
    bindings_vec.push_back(std::move(binding));

    auto result = ssp4sim::ext::ssp1::ssv::get_start_value_mappings(bindings_vec);

    // The SSM replaces the default mappings with just the target name.
    // Only the SSM target "fmu.k" appears as a map key (not the source "k").
    REQUIRE(result.size() == 1);

    {
        auto it = result.find("fmu.k");
        REQUIRE(it != result.end());
        CHECK(it->second.name == "k");
        CHECK(it->second.type == DataType::real);
        CHECK(std::holds_alternative<double>(it->second.value));
        CHECK(std::get<double>(it->second.value) == 2.0);
        CHECK(it->second.mappings.size() == 1);
        CHECK(it->second.mappings[0] == "fmu.k");
    }
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

    ssp4cpp::ssp1::ssd::ParameterBinding binding;
    ssp4cpp::ssp1::ssd::ParameterValues pv;
    pv.ParameterSet = std::move(param_set);
    binding.ParameterValues = std::move(pv);

    std::vector<ssp4cpp::ssp1::ssd::ParameterBinding> bindings_vec;
    bindings_vec.push_back(std::move(binding));

    auto result = ssp4sim::ext::ssp1::ssv::get_start_value_mappings(bindings_vec);

    REQUIRE(result.size() == 1);

    auto it = result.find("gain.k");
    REQUIRE(it != result.end());
    CHECK(it->second.name == "gain.k");
    CHECK(it->second.type == DataType::real);
    CHECK(std::holds_alternative<double>(it->second.value));
    CHECK(std::get<double>(it->second.value) == 3.0);
    CHECK(it->second.mappings.size() == 1);
    CHECK(it->second.mappings[0] == "gain.k");
}

// ---------------------------------------------------------------------------
// Test Case 4: Empty bindings
// ---------------------------------------------------------------------------
TEST_CASE("Empty bindings produce empty start values", "[parameter_binding]")
{
    std::vector<ssp4cpp::ssp1::ssd::ParameterBinding> empty;
    auto result = ssp4sim::ext::ssp1::ssv::get_start_value_mappings(empty);
    REQUIRE(result.empty());
}

// ---------------------------------------------------------------------------
// Test Case 5: get_start_value_mappings with SSM creates entries keyed by
//             mapping target names
// ---------------------------------------------------------------------------
TEST_CASE("SSM mappings produce map entries keyed by target names", "[parameter_binding]")
{
    // Create a parameter "k" with an SSM entry k -> fmu.k
    ssp4cpp::ssp1::ssv::TParameters params;
    params.Parameters = { make_real_param("k", 2.0) };

    ssp4cpp::ssp1::ssv::ParameterSet param_set;
    param_set.name = "test";
    param_set.Parameters = std::move(params);

    ssp4cpp::ssp1::ssm::TMappingEntry entry;
    entry.source = "k";
    entry.target = "fmu.k";

    ssp4cpp::ssp1::ssm::ParameterMapping ssm_mapping;
    ssm_mapping.version = "1.0";
    ssm_mapping.MappingEntry = {entry};

    ssp4cpp::ssp1::ssd::ParameterBinding binding;
    ssp4cpp::ssp1::ssd::ParameterValues pv;
    pv.ParameterSet = std::move(param_set);
    binding.ParameterValues = std::move(pv);
    ssp4cpp::ssp1::ssd::ParameterMapping pm;
    pm.ParameterMapping = std::move(ssm_mapping);
    binding.ParameterMapping = std::move(pm);

    std::vector<ssp4cpp::ssp1::ssd::ParameterBinding> bindings_vec;
    bindings_vec.push_back(std::move(binding));

    auto result = ssp4sim::ext::ssp1::ssv::get_start_value_mappings(bindings_vec);

    // Only the SSM target appears as a key, not the source name
    REQUIRE(result.size() == 1);

    auto it = result.find("fmu.k");
    REQUIRE(it != result.end());
    CHECK(it->second.name == "k");
    CHECK(it->second.type == DataType::real);
    CHECK(std::get<double>(it->second.value) == 2.0);
    CHECK(it->second.mappings.size() == 1);
    CHECK(it->second.mappings[0] == "fmu.k");
}

// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// Test Case 6: External ParameterSet (constructed), no ParameterMapping
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

    // Create ParameterBinding with SSV and no SSM (simulating external SSV source)
    ssp4cpp::ssp1::ssd::ParameterBinding binding;
    ssp4cpp::ssp1::ssd::ParameterValues pv;
    pv.ParameterSet = std::move(param_set);
    binding.ParameterValues = std::move(pv);

    std::vector<ssp4cpp::ssp1::ssd::ParameterBinding> bindings_vec;
    bindings_vec.push_back(std::move(binding));

    auto result = ssp4sim::ext::ssp1::ssv::get_start_value_mappings(bindings_vec);

    REQUIRE(result.size() == 2);

    {
        auto it = result.find("ext.k");
        REQUIRE(it != result.end());
        CHECK(it->second.name == "ext.k");
        CHECK(std::holds_alternative<double>(it->second.value));
        CHECK(std::get<double>(it->second.value) == 4.0);
        CHECK(it->second.type == DataType::real);
        CHECK(it->second.mappings.size() == 1);
        CHECK(it->second.mappings[0] == "ext.k");
    }

    {
        auto it = result.find("ext.gain");
        REQUIRE(it != result.end());
        CHECK(it->second.name == "ext.gain");
        CHECK(std::holds_alternative<double>(it->second.value));
        CHECK(std::get<double>(it->second.value) == 1.5);
        CHECK(it->second.type == DataType::real);
        CHECK(it->second.mappings.size() == 1);
        CHECK(it->second.mappings[0] == "ext.gain");
    }
}

// ---------------------------------------------------------------------------
// Test Case 7: Inline ParameterSet with external ParameterMapping (constructed)
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
    ssp4cpp::ssp1::ssm::ParameterMapping ssm_mapping;
    ssm_mapping.version = "1.0";
    ssm_mapping.MappingEntry = {entry};

    // Create ParameterBinding with both (simulating inline SSV + external SSM)
    ssp4cpp::ssp1::ssd::ParameterBinding binding;
    ssp4cpp::ssp1::ssd::ParameterValues pv;
    pv.ParameterSet = std::move(param_set);
    binding.ParameterValues = std::move(pv);
    ssp4cpp::ssp1::ssd::ParameterMapping pm;
    pm.ParameterMapping = std::move(ssm_mapping);
    binding.ParameterMapping = std::move(pm);

    std::vector<ssp4cpp::ssp1::ssd::ParameterBinding> bindings_vec;
    bindings_vec.push_back(std::move(binding));

    auto result = ssp4sim::ext::ssp1::ssv::get_start_value_mappings(bindings_vec);

    // SSM target "sink.a" is the map key; source name "a" is not a key
    REQUIRE(result.size() == 1);

    auto it = result.find("sink.a");
    REQUIRE(it != result.end());
    CHECK(it->second.name == "a");
    CHECK(it->second.type == DataType::real);
    CHECK(std::holds_alternative<double>(it->second.value));
    CHECK(std::get<double>(it->second.value) == 1.0);
    CHECK(it->second.mappings.size() == 1);
    CHECK(it->second.mappings[0] == "sink.a");
}

// ---------------------------------------------------------------------------
// Test Case 8: External ParameterSet with external ParameterMapping (constructed)
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
    ssp4cpp::ssp1::ssm::ParameterMapping ssm_mapping;
    ssm_mapping.version = "1.0";
    ssm_mapping.MappingEntry = {entry};

    // Create ParameterBinding with both (simulating all-external scenario)
    ssp4cpp::ssp1::ssd::ParameterBinding binding;
    ssp4cpp::ssp1::ssd::ParameterValues pv;
    pv.ParameterSet = std::move(param_set);
    binding.ParameterValues = std::move(pv);
    ssp4cpp::ssp1::ssd::ParameterMapping pm;
    pm.ParameterMapping = std::move(ssm_mapping);
    binding.ParameterMapping = std::move(pm);

    std::vector<ssp4cpp::ssp1::ssd::ParameterBinding> bindings_vec;
    bindings_vec.push_back(std::move(binding));

    auto result = ssp4sim::ext::ssp1::ssv::get_start_value_mappings(bindings_vec);

    REQUIRE(result.size() == 1);

    auto it = result.find("out.b");
    REQUIRE(it != result.end());
    CHECK(it->second.name == "b");
    CHECK(it->second.type == DataType::real);
    CHECK(std::holds_alternative<double>(it->second.value));
    CHECK(std::get<double>(it->second.value) == 9.0);
    CHECK(it->second.mappings.size() == 1);
    CHECK(it->second.mappings[0] == "out.b");
}

// ---------------------------------------------------------------------------
// Test Case 9: Empty external ParameterSet edge case
// ---------------------------------------------------------------------------
TEST_CASE("Empty external ParameterSet edge case", "[parameter_binding]")
{
    // Create empty TParameters (simulating external SSV with no parameters)
    ssp4cpp::ssp1::ssv::TParameters params;

    // Create ParameterSet with empty TParameters
    ssp4cpp::ssp1::ssv::ParameterSet param_set;
    param_set.name = "empty_ssv";
    param_set.Parameters = std::move(params);

    // Create ParameterBinding with empty SSV and no SSM
    ssp4cpp::ssp1::ssd::ParameterBinding binding;
    ssp4cpp::ssp1::ssd::ParameterValues pv;
    pv.ParameterSet = std::move(param_set);
    binding.ParameterValues = std::move(pv);

    std::vector<ssp4cpp::ssp1::ssd::ParameterBinding> bindings_vec;
    bindings_vec.push_back(std::move(binding));

    auto result = ssp4sim::ext::ssp1::ssv::get_start_value_mappings(bindings_vec);

    REQUIRE(result.size() == 0);
    CHECK(result.empty());
}
