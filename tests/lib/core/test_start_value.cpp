#include "pre/1_ssp_parser/schema_extensions/parameter_value.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <variant>

using ssp4sim::ext::ParameterValue;
using ssp4sim::types::DataType;

TEST_CASE("ParameterValue initializes defaults by data type", "[ParameterValue]")
{
    SECTION("real defaults to 0.0")
    {
        ParameterValue start_value("model.real", DataType::real);
        REQUIRE(start_value.name == "model.real");
        REQUIRE(std::holds_alternative<double>(start_value.value));
        REQUIRE(std::get<double>(start_value.value) == 0.0);
        REQUIRE(*reinterpret_cast<double *>(start_value.raw_ptr()) == 0.0);
    }

    SECTION("integer-like types default to 0")
    {
        ParameterValue integer_value("model.integer", DataType::integer);
        ParameterValue boolean_value("model.boolean", DataType::boolean);
        ParameterValue enum_value("model.enum", DataType::enumeration);

        REQUIRE(std::holds_alternative<int>(integer_value.value));
        REQUIRE(std::holds_alternative<int>(boolean_value.value));
        REQUIRE(std::holds_alternative<int>(enum_value.value));
        REQUIRE(std::get<int>(integer_value.value) == 0);
        REQUIRE(std::get<int>(boolean_value.value) == 0);
        REQUIRE(std::get<int>(enum_value.value) == 0);
    }

    SECTION("string defaults to empty string")
    {
        ParameterValue start_value("model.string", DataType::string);
        REQUIRE(std::holds_alternative<std::string>(start_value.value));
        REQUIRE(std::get<std::string>(start_value.value).empty());
        REQUIRE(reinterpret_cast<std::string *>(start_value.raw_ptr())->empty());
    }

    SECTION("unknown throws on construction")
    {
        REQUIRE_THROWS_AS(ParameterValue("model.unknown", DataType::unknown), std::runtime_error);
    }
}

TEST_CASE("ParameterValue stores values according to its declared data type", "[ParameterValue]")
{
    SECTION("real")
    {
        ParameterValue start_value("model.real", DataType::real);
        double input = 12.75;
        start_value.store_value(&input);
        REQUIRE(std::get<double>(start_value.value) == input);
    }

    SECTION("boolean, integer and enumeration")
    {
        int boolean_input = 1;
        int integer_input = -42;
        int enum_input = 7;

        ParameterValue boolean_value("model.boolean", DataType::boolean);
        ParameterValue integer_value("model.integer", DataType::integer);
        ParameterValue enum_value("model.enum", DataType::enumeration);

        boolean_value.store_value(&boolean_input);
        integer_value.store_value(&integer_input);
        enum_value.store_value(&enum_input);

        REQUIRE(std::get<int>(boolean_value.value) == boolean_input);
        REQUIRE(std::get<int>(integer_value.value) == integer_input);
        REQUIRE(std::get<int>(enum_value.value) == enum_input);
    }

    SECTION("string")
    {
        ParameterValue start_value("model.string", DataType::string);
        std::string input = "hello";
        start_value.store_value(&input);
        REQUIRE(std::get<std::string>(start_value.value) == input);
    }

    SECTION("unknown throws on construction")
    {
        REQUIRE_THROWS_AS(ParameterValue("model.unknown", DataType::unknown), std::runtime_error);
    }
}

TEST_CASE("ParameterValue exposes mutable and const raw pointers", "[ParameterValue]")
{
    ParameterValue start_value("model.real", DataType::real);
    double input = 3.25;
    start_value.store_value(&input);

    auto *mutable_ptr = reinterpret_cast<double *>(start_value.raw_ptr());
    REQUIRE(mutable_ptr != nullptr);
    REQUIRE(*mutable_ptr == 3.25);

    const auto &const_start_value = static_cast<const ParameterValue &>(start_value);
    auto *const_ptr = reinterpret_cast<const double *>(const_start_value.raw_ptr());
    REQUIRE(const_ptr != nullptr);
    REQUIRE(*const_ptr == 3.25);
}

TEST_CASE("ParameterValue to_string includes key value information", "[ParameterValue]")
{
    SECTION("real value rendering")
    {
        ParameterValue start_value("plant.gain", DataType::real);
        double input = 2.0;
        start_value.store_value(&input);

        const auto rendered = start_value.to_string();
        REQUIRE(rendered.find("Name: plant.gain") != std::string::npos);
        REQUIRE(rendered.find("Value: 2.000000") != std::string::npos);
    }

    SECTION("unknown value rendering")
    {
        REQUIRE_THROWS_AS(ParameterValue("plant.unknown", DataType::unknown), std::runtime_error);
    }
}

TEST_CASE("ParameterValue supports copy construction", "[ParameterValue]")
{
    ParameterValue original("copy.source", DataType::string);
    std::string input = "copied";
    original.store_value(&input);

    ParameterValue copy(original);

    REQUIRE(copy.name == original.name);
    REQUIRE(copy.type == original.type);
    REQUIRE(std::holds_alternative<std::string>(copy.value));
    REQUIRE(std::get<std::string>(copy.value) == "copied");
    REQUIRE(copy.raw_ptr() != original.raw_ptr());
}

TEST_CASE("ParameterValue supports copy assignment", "[ParameterValue]")
{
    ParameterValue source("assign.source", DataType::real);
    double source_input = 9.5;
    source.store_value(&source_input);

    ParameterValue target("assign.target", DataType::integer);
    int target_input = 123;
    target.store_value(&target_input);

    target = source;

    REQUIRE(target.name == source.name);
    REQUIRE(target.type == source.type);
    REQUIRE(std::holds_alternative<double>(target.value));
    REQUIRE(std::get<double>(target.value) == 9.5);
    REQUIRE(target.raw_ptr() != source.raw_ptr());

    target = target;
    REQUIRE(target.name == "assign.source");
    REQUIRE(target.type == DataType::real);
    REQUIRE(std::get<double>(target.value) == 9.5);
}
