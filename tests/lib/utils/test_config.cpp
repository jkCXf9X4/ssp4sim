
#include <catch2/catch_test_macros.hpp>

#include "utils/config.hpp"
#include "shared_config.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <filesystem>
#include <string>

using namespace ssp4sim::utils;
namespace fs = std::filesystem;

TEST_CASE("Config tests", "[config]")
{
    const fs::path project_root{SSP4SIM_PROJECT_ROOT};
    const std::string config_file = (project_root / "tests" / "resources" / "references" / "test_config.json").string();
    const std::string malformed_config_file = (project_root / "tests" / "resources"  / "references" / "malformed_test_config.json").string();

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

    SECTION("Recording config is parsed")
    {
        Config::loadFromString(R"json({
            "simulation": {
                "ssp": "./fake.ssp",
                "working_dir": "./wd/test",
                "start_time": 0.0,
                "stop_time": 1.0,
                "timestep": 0.1,
                "recording": {
                    "csv": {
                        "enable": true,
                        "interval": 0.5
                    },
                    "parquet": {
                        "enable": true
                    }
                }
            }
        })json");

        auto *log = ssp4cpp::utils::log::simple_logger();
        ssp4sim::SharedConfig shared_config(log);

        REQUIRE(shared_config.enable_recording);
        REQUIRE(shared_config.csv.enable);
        REQUIRE(shared_config.csv.file == fs::path("./wd/test/result.csv"));
        REQUIRE(shared_config.csv.interval == ssp4sim::utils::time::s_to_ns(0.5));
        REQUIRE(shared_config.parquet.enable);
        REQUIRE(shared_config.parquet.file == fs::path("./wd/test/result.parquet"));
    }

    SECTION("Recording config can be disabled")
    {
        Config::loadFromString(R"json(
        {
            "simulation": {
                "ssp": "./fake.ssp",
                "start_time": 0.0,
                "stop_time": 1.0,
                "timestep": 0.1,
                "recording": {
                    "csv": {
                        "enable": false
                    }
                }
            }
        }
        )json");

        auto *log = ssp4cpp::utils::log::simple_logger();
        ssp4sim::SharedConfig shared_config(log);

        REQUIRE_FALSE(shared_config.enable_recording);
        REQUIRE_FALSE(shared_config.csv.enable);
        REQUIRE_FALSE(shared_config.parquet.enable);
    }

}
