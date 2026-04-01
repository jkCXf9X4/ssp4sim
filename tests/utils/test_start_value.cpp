#include "initial_value.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>
#include <variant>

using ssp4sim::ext::ssp1::ssv::StartValue;
using ssp4sim::types::DataType;

TEST_CASE("StartValue initializes defaults and mappings by data type", "[StartValue]")
{
    SECTION("real defaults to 0.0")
    {
        StartValue start_value("model.real", DataType::real);
        REQUIRE(start_value.name == "model.real");
        REQUIRE(start_value.mappings.size() == 1);
        REQUIRE(start_value.mappings.front() == "model.real");
        REQUIRE(std::holds_alternative<double>(start_value.value));
        REQUIRE(std::get<double>(start_value.value) == 0.0);
        REQUIRE(*reinterpret_cast<double *>(start_value.raw_ptr()) == 0.0);
    }

    SECTION("integer-like types default to 0")
    {
        StartValue integer_value("model.integer", DataType::integer);
        StartValue boolean_value("model.boolean", DataType::boolean);
        StartValue enum_value("model.enum", DataType::enumeration);

        REQUIRE(std::holds_alternative<int>(integer_value.value));
        REQUIRE(std::holds_alternative<int>(boolean_value.value));
        REQUIRE(std::holds_alternative<int>(enum_value.value));
        REQUIRE(std::get<int>(integer_value.value) == 0);
        REQUIRE(std::get<int>(boolean_value.value) == 0);
        REQUIRE(std::get<int>(enum_value.value) == 0);
    }

    SECTION("string defaults to empty string")
    {
        StartValue start_value("model.string", DataType::string);
        REQUIRE(std::holds_alternative<std::string>(start_value.value));
        REQUIRE(std::get<std::string>(start_value.value).empty());
        REQUIRE(reinterpret_cast<std::string *>(start_value.raw_ptr())->empty());
    }

    SECTION("unknown defaults to monostate and null raw pointer")
    {
        StartValue start_value("model.unknown", DataType::unknown);
        REQUIRE(std::holds_alternative<std::monostate>(start_value.value));
        REQUIRE(start_value.raw_ptr() == nullptr);
        REQUIRE(static_cast<const StartValue &>(start_value).raw_ptr() == nullptr);
    }
}

TEST_CASE("StartValue stores values according to its declared data type", "[StartValue]")
{
    SECTION("real")
    {
        StartValue start_value("model.real", DataType::real);
        double input = 12.75;
        start_value.store_value(&input);
        REQUIRE(std::get<double>(start_value.value) == input);
    }

    SECTION("boolean, integer and enumeration")
    {
        int boolean_input = 1;
        int integer_input = -42;
        int enum_input = 7;

        StartValue boolean_value("model.boolean", DataType::boolean);
        StartValue integer_value("model.integer", DataType::integer);
        StartValue enum_value("model.enum", DataType::enumeration);

        boolean_value.store_value(&boolean_input);
        integer_value.store_value(&integer_input);
        enum_value.store_value(&enum_input);

        REQUIRE(std::get<int>(boolean_value.value) == boolean_input);
        REQUIRE(std::get<int>(integer_value.value) == integer_input);
        REQUIRE(std::get<int>(enum_value.value) == enum_input);
    }

    SECTION("string")
    {
        StartValue start_value("model.string", DataType::string);
        std::string input = "hello";
        start_value.store_value(&input);
        REQUIRE(std::get<std::string>(start_value.value) == input);
    }

    SECTION("unknown remains monostate")
    {
        StartValue start_value("model.unknown", DataType::unknown);
        int input = 99;
        start_value.store_value(&input);
        REQUIRE(std::holds_alternative<std::monostate>(start_value.value));
        REQUIRE(start_value.raw_ptr() == nullptr);
    }
}

TEST_CASE("StartValue exposes mutable and const raw pointers", "[StartValue]")
{
    StartValue start_value("model.real", DataType::real);
    double input = 3.25;
    start_value.store_value(&input);

    auto *mutable_ptr = reinterpret_cast<double *>(start_value.raw_ptr());
    REQUIRE(mutable_ptr != nullptr);
    REQUIRE(*mutable_ptr == 3.25);

    const auto &const_start_value = static_cast<const StartValue &>(start_value);
    auto *const_ptr = reinterpret_cast<const double *>(const_start_value.raw_ptr());
    REQUIRE(const_ptr != nullptr);
    REQUIRE(*const_ptr == 3.25);
}

TEST_CASE("StartValue to_string includes key value information", "[StartValue]")
{
    SECTION("real value rendering")
    {
        StartValue start_value("plant.gain", DataType::real);
        double input = 2.0;
        start_value.store_value(&input);

        const auto rendered = start_value.to_string();
        REQUIRE(rendered.find("Name: plant.gain") != std::string::npos);
        REQUIRE(rendered.find("Value: 2.000000") != std::string::npos);
    }

    SECTION("unknown value rendering")
    {
        StartValue start_value("plant.unknown", DataType::unknown);

        const auto rendered = start_value.to_string();
        REQUIRE(rendered.find("Name: plant.unknown") != std::string::npos);
        REQUIRE(rendered.find("Value: <bin>") != std::string::npos);
    }
}

TEST_CASE("StartValue supports copy construction", "[StartValue]")
{
    StartValue original("copy.source", DataType::string);
    original.mappings.push_back("alias.source");
    std::string input = "copied";
    original.store_value(&input);

    StartValue copy(original);

    REQUIRE(copy.name == original.name);
    REQUIRE(copy.type == original.type);
    REQUIRE(copy.mappings == original.mappings);
    REQUIRE(std::holds_alternative<std::string>(copy.value));
    REQUIRE(std::get<std::string>(copy.value) == "copied");
    REQUIRE(copy.raw_ptr() != original.raw_ptr());
}

TEST_CASE("StartValue supports copy assignment", "[StartValue]")
{
    StartValue source("assign.source", DataType::real);
    source.mappings.push_back("assign.alias");
    double source_input = 9.5;
    source.store_value(&source_input);

    StartValue target("assign.target", DataType::unknown);
    int target_input = 123;
    target.store_value(&target_input);

    target = source;

    REQUIRE(target.name == source.name);
    REQUIRE(target.type == source.type);
    REQUIRE(target.mappings == source.mappings);
    REQUIRE(std::holds_alternative<double>(target.value));
    REQUIRE(std::get<double>(target.value) == 9.5);
    REQUIRE(target.raw_ptr() != source.raw_ptr());

    target = target;
    REQUIRE(target.name == "assign.source");
    REQUIRE(target.type == DataType::real);
    REQUIRE(std::get<double>(target.value) == 9.5);
}
