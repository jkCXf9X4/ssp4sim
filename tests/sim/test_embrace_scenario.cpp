#include <catch2/catch_test_macros.hpp>

#include "simulator.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace
{
    namespace fs = std::filesystem;

    TEST_CASE("embrace scenario generates stable results", "[integration][embrace]")
    {
        const fs::path project_root{SSP4SIM_PROJECT_ROOT};

        const fs::path base_config = project_root / "resources" / "integrations_tests" / "it_embrace.json";
        const fs::path ssp_path = project_root / "resources" / "embrace" / "embrace_scen.ssp";
        REQUIRE(fs::exists(base_config));
        REQUIRE(fs::exists(ssp_path));

        ssp4sim::Simulator simulator(base_config.string());
        simulator.init();
        simulator.simulate();
    }

}
