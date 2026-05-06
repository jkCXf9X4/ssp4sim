
#include <catch2/catch_test_macros.hpp>

#include "utils/config.hpp"
#include "shared_config.hpp"

#include "ssp4cpp/utils/log.hpp"

#include <cstdlib>
#include <chrono>
#include <fstream>
#include <filesystem>
#include <string>

using namespace ssp4sim::utils;
namespace fs = std::filesystem;

namespace
{
    class ScopedEnvVar
    {
    public:
        ScopedEnvVar(const char *name, const char *value)
            : name(name)
        {
            if (const char *current = std::getenv(name); current != nullptr)
            {
                has_original = true;
                original = current;
            }

            if (setenv(this->name.c_str(), value, 1) != 0)
            {
                throw std::runtime_error("failed to set environment variable");
            }
        }

        ~ScopedEnvVar()
        {
            if (has_original)
            {
                setenv(name.c_str(), original.c_str(), 1);
            }
            else
            {
                unsetenv(name.c_str());
            }
        }

    private:
        std::string name;
        std::string original;
        bool has_original = false;
    };
}

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
        Config::loadFromString(R"json(
        {
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
                    "influx": {
                        "enable": true,
                        "url": "http://localhost:8086",
                        "db": "ssp4sim",
                        "token": "config-token",
                        "measurement": "signals",
                        "run": "run-[TIME]",
                        "batch_size": 123,
                        "interval": 0.25
                }
            }
        }
        }
        )json");

        auto *log = ssp4cpp::utils::log::simple_logger();
        ssp4sim::SharedConfig shared_config(log);

        REQUIRE(shared_config.enable_recording);
        REQUIRE(shared_config.csv.enable);
        REQUIRE(shared_config.csv.file == fs::path("./wd/test/result.csv"));
        REQUIRE(shared_config.csv.interval == ssp4sim::utils::time::s_to_ns(0.5));
        REQUIRE(shared_config.influx.enable);
        REQUIRE(shared_config.influx.url == "http://localhost:8086");
        REQUIRE(shared_config.influx.db == "ssp4sim");
        REQUIRE(shared_config.influx.token == "config-token");
        REQUIRE(shared_config.influx.measurement == "signals");
        REQUIRE(shared_config.influx.batch_size == 123);
        REQUIRE(shared_config.influx.interval == ssp4sim::utils::time::s_to_ns(0.25));
        REQUIRE(shared_config.influx.run.rfind("run-", 0) == 0);
    }

    SECTION("Influx recording token falls back to environment")
    {
        [[maybe_unused]] ScopedEnvVar token_env("SSP4SIM_INFLUX_TOKEN", "env-token");

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
                    },
                    "influx": {
                        "enable": true,
                        "url": "http://localhost:8181",
                        "db": "ssp4sim"
                    }
                }
            }
        }
        )json");

        auto *log = ssp4cpp::utils::log::simple_logger();
        ssp4sim::SharedConfig shared_config(log);

        REQUIRE(shared_config.influx.token == "env-token");
    }

    SECTION("Influx recording token falls back to the local config file")
    {
        const fs::path temp_home = fs::temp_directory_path() /
                                   ("ssp4sim-influx-home-" +
                                    std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        const fs::path config_dir = temp_home / ".influxdb" / "docker" / "explorer" / "config";
        fs::create_directories(config_dir);

        const fs::path config_file_path = config_dir / "config.json";
        {
            std::ofstream config_file(config_file_path);
            config_file << R"json({"DEFAULT_API_TOKEN":"file-token"})json";
        }

        const std::string home_str = temp_home.string();
        [[maybe_unused]] ScopedEnvVar home_env("HOME", home_str.c_str());

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
                    },
                    "influx": {
                        "enable": true,
                        "url": "http://localhost:8181",
                        "db": "ssp4sim"
                    }
                }
            }
        }
        )json");

        auto *log = ssp4cpp::utils::log::simple_logger();
        ssp4sim::SharedConfig shared_config(log);

        REQUIRE(shared_config.influx.token == "file-token");

        fs::remove_all(temp_home);
    }

}
