#include <catch2/catch_test_macros.hpp>

#include "simulator.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <string>

namespace
{
    namespace fs = std::filesystem;

    fs::path project_root()
    {
        return fs::path(SSP4SIM_PROJECT_ROOT);
    }

    fs::path smoke_ssp_root()
    {
        return project_root() / "tests" / "reference_ssp" / "build" / "models" / "pyfmu_csv_source_sink" / "ssp";
    }

    fs::path write_smoke_config(const fs::path &workdir)
    {
        const auto config_template = project_root() / "resources" / "generic_config.json";
        REQUIRE(fs::exists(config_template));
        REQUIRE(fs::exists(smoke_ssp_root() / "SystemStructure.ssd"));

        std::ifstream input(config_template);
        REQUIRE(input.is_open());

        nlohmann::json config;
        input >> config;

        config["simulation"]["ssp"] = smoke_ssp_root().string();
        config["simulation"]["ssd"] = "SystemStructure.ssd";
        config["simulation"]["start_time"] = 0.0;
        config["simulation"]["stop_time"] = 1.0;
        config["simulation"]["timestep"] = 0.1;
        config["simulation"]["tolerance"] = 1e-4;
        config["simulation"]["realtime"] = false;

        config["simulation"]["recording"]["enable"] = true;
        config["simulation"]["recording"]["wait_for"] = true;
        config["simulation"]["recording"]["interval"] = 0.1;
        config["simulation"]["recording"]["result_file"] = (workdir / "results.csv").string();

        config["simulation"]["log"]["file"] = (workdir / "sim.log").string();
        config["simulation"]["log"]["level_terminal"] = "error";
        config["simulation"]["log"]["level_file"] = "info";
        config["simulation"]["log"]["level_json"] = "info";

        fs::create_directories(workdir);
        const auto config_path = workdir / "generic_config.json";

        std::ofstream output(config_path, std::ios::trunc);
        REQUIRE(output.is_open());
        output << config.dump(2);

        return config_path;
    }
}

TEST_CASE("Simulator smoke test runs one complete unpacked SSP", "[high_level][smoke]")
{
    const auto workdir = project_root() / "build" / "test_cpp_high_level" / "pyfmu_csv_source_sink";
    const auto config_path = write_smoke_config(workdir);

    ssp4sim::Simulator simulator(config_path.string());
    simulator.init();
    simulator.simulate();

    REQUIRE(fs::exists(workdir / "results.csv"));
}
