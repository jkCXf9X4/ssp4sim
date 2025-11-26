
#include <catch2/catch_test_macros.hpp>

#include "utils/config.hpp"

#include <fstream>
#include <filesystem>

using namespace ssp4sim::utils;
namespace fs = std::filesystem;

TEST_CASE("Config tests", "[config]")
{
    const fs::path project_root{SSP4SIM_PROJECT_ROOT};
    const std::string config_file = (project_root / "tests" / "references" / "test_config.json").string();
    const std::string malformed_config_file = (project_root / "tests" / "references" / "malformed_test_config.json").string();

    SECTION("Load from file")
    {
        REQUIRE_NOTHROW(Config::loadFromFile(config_file));
    }

    SECTION("Load from non-existent file")
    {
        REQUIRE_THROWS(Config::loadFromFile("non_existent_file.json"));
    }

    SECTION("Load from malformed file")
    {
        REQUIRE_THROWS(Config::loadFromFile(malformed_config_file));
    }

    // Load the config for the following tests
    Config::loadFromFile(config_file);

    SECTION("Get required value")
    {
        REQUIRE(Config::getString("name") == "test");
        REQUIRE(Config::getInt("value") == 123);
        REQUIRE(Config::getString("nested.key") == "value");
        REQUIRE(Config::getString("array.0") == "a");
    }

    SECTION("Get required value that does not exist")
    {
        REQUIRE_THROWS(Config::getString("non_existent_key"));
    }

    SECTION("Get required value with type mismatch")
    {
        REQUIRE_THROWS(Config::getInt("name"));
    }

    SECTION("Get with default value")
    {
        REQUIRE(Config::getOr("name", std::string("default")) == "test");
        REQUIRE(Config::getOr("value", 456) == 123);
        REQUIRE(Config::getOr("nested.key", std::string("default")) == "value");
        REQUIRE(Config::getOr("array.0", std::string("default")) == "a");
    }

    SECTION("Get with default value that does not exist")
    {
        REQUIRE(Config::getOr("non_existent_key", std::string("default")) == "default");
    }

    SECTION("Get with default value with type mismatch")
    {
        REQUIRE_THROWS(Config::getOr("name", 456));
    }

    SECTION("Resolve path")
    {
        REQUIRE(Config::resolvePath("name") != nullptr);
        REQUIRE(Config::resolvePath("nested.key") != nullptr);
        REQUIRE(Config::resolvePath("array.0") != nullptr);
        REQUIRE(Config::resolvePath("non_existent_key") == nullptr);
    }

}
