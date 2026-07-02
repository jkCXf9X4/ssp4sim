#include "pre/1_ssp_parser/schema_extensions/FMI2_Enums_Ext.hpp"
#include "pre/1_ssp_parser/schema_extensions/parameter_value.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstring>
#include <string>

using ssp4sim::ext::ParameterValue;
using ssp4sim::ext::fmi2::enums::DefaultValue;
using ssp4sim::ext::fmi2::enums::get_data_type_size;
using ssp4sim::ext::fmi2::enums::get_default_value;
using ssp4sim::types::DataType;

// ---------------------------------------------------------------------------
// Description: Verifies get_data_type_size returns correct sizeof for all types
// Rationale:   Size mapping required for memory allocation and serialization
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// get_data_type_size
// ---------------------------------------------------------------------------
TEST_CASE("get_data_type_size returns correct sizes", "[schema_extensions]")
{
    CHECK(get_data_type_size(DataType::real) == sizeof(double));
    CHECK(get_data_type_size(DataType::integer) == sizeof(int));
    CHECK(get_data_type_size(DataType::boolean) == sizeof(int));
    CHECK(get_data_type_size(DataType::enumeration) == sizeof(int));
    CHECK(get_data_type_size(DataType::string) == sizeof(std::string));
    CHECK(get_data_type_size(DataType::unknown) == 0);
}

// ---------------------------------------------------------------------------
// Description: Verifies get_default_value returns FMI 2.0 defaults per type
// Rationale:   Default values must match FMI 2.0 specification
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// get_default_value
// ---------------------------------------------------------------------------
TEST_CASE("get_default_value returns correct defaults", "[schema_extensions]")
{
    SECTION("Real defaults to 0.0")
    {
        auto dv = get_default_value(DataType::real);
        REQUIRE(std::holds_alternative<double>(dv));
        CHECK(std::get<double>(dv) == 0.0);
    }

    SECTION("Integer defaults to 0")
    {
        auto dv = get_default_value(DataType::integer);
        REQUIRE(std::holds_alternative<int>(dv));
        CHECK(std::get<int>(dv) == 0);
    }

    SECTION("Boolean defaults to false")
    {
        auto dv = get_default_value(DataType::boolean);
        REQUIRE(std::holds_alternative<bool>(dv));
        CHECK(std::get<bool>(dv) == false);
    }

    SECTION("Enumeration defaults to 0")
    {
        auto dv = get_default_value(DataType::enumeration);
        REQUIRE(std::holds_alternative<int>(dv));
        CHECK(std::get<int>(dv) == 0);
    }

    SECTION("String defaults to empty string_view")
    {
        auto dv = get_default_value(DataType::string);
        REQUIRE(std::holds_alternative<std::string_view>(dv));
        CHECK(std::get<std::string_view>(dv).empty());
    }

    SECTION("Unknown throws")
    {
        REQUIRE_THROWS_AS(get_default_value(DataType::unknown), std::runtime_error);
    }
}

// ---------------------------------------------------------------------------
// Description: Verifies ParameterValue edge cases (empty name, long name,
//              raw_ptr round-trip, overwrite, default construction)
// Rationale:   Robustness — empty names appear in boundary connections;
//              long names appear in deeply nested SSPs
// Creep flag:  "Long name" test uses a qualitative string, not a boundary value
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// ParameterValue edge cases
// ---------------------------------------------------------------------------
TEST_CASE("ParameterValue edge cases", "[schema_extensions]")
{
    SECTION("Empty name is allowed")
    {
        ParameterValue pv("", DataType::real);
        CHECK(pv.name.empty());
        CHECK(pv.type == DataType::real);
    }

    SECTION("Long name is preserved")
    {
        std::string long_name = "very.deeply.nested.parameter.path.value";
        ParameterValue pv(long_name, DataType::integer);
        CHECK(pv.name == long_name);
    }

    SECTION("Store and retrieve real value via raw_ptr")
    {
        ParameterValue pv("test.val", DataType::real);
        double input = 3.14159;
        pv.store_value(&input);

        auto *ptr = reinterpret_cast<double *>(pv.raw_ptr());
        REQUIRE(ptr != nullptr);
        CHECK(*ptr == 3.14159);
    }

    SECTION("Store and retrieve integer value via raw_ptr")
    {
        ParameterValue pv("test.count", DataType::integer);
        int input = 42;
        pv.store_value(&input);

        auto *ptr = reinterpret_cast<int *>(pv.raw_ptr());
        REQUIRE(ptr != nullptr);
        CHECK(*ptr == 42);
    }

    SECTION("Store and retrieve string value via raw_ptr")
    {
        ParameterValue pv("test.name", DataType::string);
        std::string input = "hello_world";
        pv.store_value(&input);

        auto *ptr = reinterpret_cast<std::string *>(pv.raw_ptr());
        REQUIRE(ptr != nullptr);
        CHECK(*ptr == "hello_world");
    }

    SECTION("Multiple store_value calls overwrite")
    {
        ParameterValue pv("test.val", DataType::real);
        double first = 1.0;
        double second = 2.0;
        pv.store_value(&first);
        pv.store_value(&second);

        CHECK(std::get<double>(pv.value) == 2.0);
    }

    SECTION("Default-constructed value has correct type defaults")
    {
        ParameterValue real_val("r", DataType::real);
        CHECK(std::holds_alternative<double>(real_val.value));
        CHECK(std::get<double>(real_val.value) == 0.0);

        ParameterValue int_val("i", DataType::integer);
        CHECK(std::holds_alternative<int>(int_val.value));
        CHECK(std::get<int>(int_val.value) == 0);

        ParameterValue str_val("s", DataType::string);
        CHECK(std::holds_alternative<std::string>(str_val.value));
        CHECK(std::get<std::string>(str_val.value).empty());
    }
}

// ---------------------------------------------------------------------------
// Description: Verifies data_type_to_string produces readable output per type
// Rationale:   Debug/logging utility
// Creep flag:  Presentation detail — format may change without behavioral impact
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// data_type_to_string
// ---------------------------------------------------------------------------
TEST_CASE("data_type_to_string produces readable output", "[schema_extensions]")
{
    SECTION("Real value")
    {
        double val = 3.14;
        auto str = ssp4sim::ext::fmi2::enums::data_type_to_string(DataType::real, &val);
        CHECK(str.find("3.14") != std::string::npos);
    }

    SECTION("Integer value")
    {
        int val = 42;
        auto str = ssp4sim::ext::fmi2::enums::data_type_to_string(DataType::integer, &val);
        CHECK(str.find("42") != std::string::npos);
    }

    SECTION("String value")
    {
        std::string val = "test";
        auto str = ssp4sim::ext::fmi2::enums::data_type_to_string(DataType::string, &val);
        CHECK(str.find("test") != std::string::npos);
    }

    SECTION("Boolean value")
    {
        int val_true = 1;
        auto str = ssp4sim::ext::fmi2::enums::data_type_to_string(DataType::boolean, &val_true);
        CHECK(str.find("1") != std::string::npos);

        int val_false = 0;
        str = ssp4sim::ext::fmi2::enums::data_type_to_string(DataType::boolean, &val_false);
        CHECK(str.find("0") != std::string::npos);
    }
}