#include <catch2/catch_test_macros.hpp>

#include "simulator.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace
{
    namespace fs = std::filesystem;

    fs::path project_root()
    {
        return fs::path(SSP4SIM_PROJECT_ROOT);
    }

    fs::path reference_models_root()
    {
        return project_root() / "tests" / "reference_ssp" / "build" / "models";
    }

    bool uses_model_exchange(const fs::path &model_root)
    {
        const auto ssd_path = model_root / "ssp" / "SystemStructure.ssd";
        std::ifstream input(ssd_path);
        REQUIRE(input.is_open());

        const std::string contents((std::istreambuf_iterator<char>(input)),
                                   std::istreambuf_iterator<char>());

        return contents.find("implementation=\"ModelExchange\"") != std::string::npos ||
               contents.find("implementation='ModelExchange'") != std::string::npos;
    }

    std::vector<fs::path> discover_reference_ssps()
    {
        std::vector<fs::path> models;

        for (const auto &entry : fs::directory_iterator(reference_models_root()))
        {
            if (!entry.is_directory())
            {
                continue;
            }

            const auto ssp_root = entry.path() / "ssp";
            if (!fs::exists(ssp_root / "SystemStructure.ssd"))
            {
                continue;
            }

            if (uses_model_exchange(entry.path()))
            {
                continue;
            }

            models.push_back(entry.path());
        }

        std::sort(models.begin(), models.end());
        return models;
    }

    fs::path write_reference_config(const fs::path &model_root, const fs::path &workdir)
    {
        const auto config_template = project_root() / "resources" / "generic_config.json";
        REQUIRE(fs::exists(config_template));

        std::ifstream input(config_template);
        REQUIRE(input.is_open());

        nlohmann::json config_json;
        input >> config_json;

        config_json["simulation"]["ssp"] = (model_root / "ssp").string();
        config_json["simulation"]["ssd"] = "SystemStructure.ssd";
        config_json["simulation"]["start_time"] = 0.0;
        config_json["simulation"]["stop_time"] = 1.0;
        config_json["simulation"]["timestep"] = 0.1;
        config_json["simulation"]["tolerance"] = 1e-4;
        config_json["simulation"]["recording"]["enable"] = true;
        config_json["simulation"]["recording"]["result_file"] = (workdir / "results.csv").string();
        config_json["simulation"]["log"]["file"] = (workdir / "sim.log").string();

        fs::create_directories(workdir);
        const auto config_path = workdir / "generic_config.json";

        std::ofstream output(config_path, std::ios::trunc);
        REQUIRE(output.is_open());
        output << config_json.dump(2);

        return config_path;
    }

    TEST_CASE("test_references fully simulates available co-simulation SSPs", "[integration][references]")
    {
        const auto models = discover_reference_ssps();
        REQUIRE_FALSE(models.empty());

        const auto work_root = project_root() / "build" / "test_references";

        for (const auto &model_root : models)
        {
            const auto model_name = model_root.filename().string();

            DYNAMIC_SECTION(model_name)
            {
                const auto config_path = write_reference_config(model_root, work_root / model_name);

                ssp4sim::Simulator simulator(config_path.string());
                simulator.init();
                simulator.simulate();
            }
        }
    }
}
