#include <catch2/catch_test_macros.hpp>

#include "simulator.hpp"

#include <nlohmann/json.hpp>

#include <filesystem>
#include <fstream>
#include <algorithm>
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
        return project_root() / "resources" / "reference_ssp" / "artifacts" / "models" / "pyfmu_csv_source_sink" / "baseline";
    }

    fs::path runtime_smoke_ssp_root(const fs::path &workdir)
    {
        const auto runtime_ssp_root = workdir / "ssp";
        fs::remove_all(runtime_ssp_root);
        fs::copy(
            smoke_ssp_root(),
            runtime_ssp_root,
            fs::copy_options::recursive | fs::copy_options::copy_symlinks);
        return runtime_ssp_root;
    }

    fs::path write_smoke_config(const fs::path &workdir)
    {
        const auto config_template = project_root() / "resources" / "generic_config.json";
        REQUIRE(fs::exists(config_template));

        fs::create_directories(workdir);

        std::ifstream input(config_template);
        REQUIRE(input.is_open());

        const auto ssp_root = runtime_smoke_ssp_root(workdir);
        REQUIRE(fs::exists(ssp_root / "SystemStructure.ssd"));

        nlohmann::json config;
        input >> config;

        config["simulation"]["ssp"] = ssp_root.string();
        config["simulation"]["ssd"] = "SystemStructure.ssd";
        config["simulation"]["start_time"] = 0.0;
        config["simulation"]["stop_time"] = 1.0;
        config["simulation"]["timestep"] = 0.1;
        config["simulation"]["tolerance"] = 1e-4;
        config["simulation"]["realtime"] = false;
        config["simulation"]["working_dir"] = workdir.string();

        config["simulation"]["recording"]["enable"] = true;
        config["simulation"]["recording"]["wait_for"] = true;
        config["simulation"]["recording"]["interval"] = 0.1;

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

// ---------------------------------------------------------------------------
// Description: Full pipeline: copy fixture, write config, init, simulate,
//              verify result.csv and sim*.log exist
// Rationale:   Top-level smoke test exercising entire simulator entry point.
//              Per tests/README.md, this should be the only C++ high-level test.
// ---------------------------------------------------------------------------
TEST_CASE("Simulator smoke test runs one complete unpacked SSP", "[high_level][smoke]")
{
    const auto workdir = project_root() / "build" / "test_cpp_high_level" / "pyfmu_csv_source_sink";
    const auto config_path = write_smoke_config(workdir);

    ssp4sim::Simulator simulator(config_path.string());
    simulator.init();
    simulator.simulate();

    REQUIRE(fs::exists(workdir / "result.csv"));
    const auto has_log_file = std::any_of(
        fs::directory_iterator(workdir),
        fs::directory_iterator{},
        [](const fs::directory_entry &entry)
        {
            const auto name = entry.path().filename().string();
            return entry.is_regular_file() && name.rfind("sim", 0) == 0 && entry.path().extension() == ".log";
        });
    REQUIRE(has_log_file);
}
