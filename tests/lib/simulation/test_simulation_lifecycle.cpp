#include "simulation.hpp"
#include "shared_config.hpp"

#include "utils/config.hpp"

#include "ssp4cpp/ssp.hpp"
#include "ssp4cpp/utils/log.hpp"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <stdexcept>
#include <string>

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------
// Description: Verifies the Simulation lifecycle guards:
//                (a) simulate() before init() throws std::runtime_error
//                (b) a second init() throws std::logic_error
// Rationale:   Both callers (Simulator::init, simulator_c_api) call init()
//              exactly once per lifecycle; a second init() is a programming
//              error and must fail loudly instead of silently re-initializing.
// Fixture note: (b) needs a successful first init(), which requires a real
//              SSP. Ssp's ctor takes an archive path (zip or directory);
//              there is no in-memory ctor and no .ssp archive in
//              tests/resources (only .ssd reference files). The test uses the
//              directory-based SSP fixture pattern from
//              test_graph_builder.cpp
//              (resources/reference_ssp/artifacts/models/<name>/baseline) and
//              SKIPs when the fixture is not present, so the suite stays
//              green without a .ssp archive.
// ---------------------------------------------------------------------------

namespace {

    fs::path project_root()
    {
        return fs::path(SSP4SIM_PROJECT_ROOT);
    }

    fs::path fixture_path(const std::string &fixture_name)
    {
        return project_root() / "resources" / "reference_ssp" / "artifacts" / "models" /
               fixture_name / "baseline";
    }

    /// Minimal SharedConfig with recording disabled (csv + sqlite off).
    /// The SharedConfig ctor reads required keys (simulation.ssp,
    /// start_time, stop_time, timestep) from the global utils::Config, so a
    /// minimal JSON config must be loaded first (same pattern as
    /// test_config.cpp). The Simulation ctor only stores ssp/config and sets
    /// up recorder sinks based on enable_recording; with recording disabled
    /// it is sufficient for the lifecycle tests.
    ssp4sim::SharedConfig make_minimal_config()
    {
        ssp4sim::utils::Config::loadFromString(R"json(
        {
            "simulation": {
                "ssp": "./fake.ssp",
                "start_time": 0.0,
                "stop_time": 1.0,
                "timestep": 0.1,
                "tolerance": 1e-6,
                "executor": { "forward_derivatives": false },
                "log": { "fmu": false },
                "recording": {
                    "csv": {
                        "enable": false
                    },
                    "sqlite": {
                        "enable": false
                    }
                }
            }
        }
        )json");

        auto *log = ssp4cpp::utils::log::simple_logger();
        ssp4sim::SharedConfig config(log);
        config.enable_recording = false;
        config.csv.enable = false;
        config.sqlite.enable = false;
        config.working_dir = project_root() / ".dynamic-harness" / "wd" / "sim_lifecycle_test";
        return config;
    }

} // anonymous namespace

// ---------------------------------------------------------------------------
// (a) simulate() before init() throws
// ---------------------------------------------------------------------------
TEST_CASE("simulate before init throws", "[simulation][lifecycle]")
{
    auto config = make_minimal_config();

    // Null ssp pointer is fine here: simulate() must throw on the missing
    // sim_graph guard before touching the ssp.
    ssp4sim::Simulation sim(nullptr, &config);

    try
    {
        sim.simulate();
        FAIL("simulate() before init() should have thrown");
    }
    catch (const std::runtime_error &e)
    {
        REQUIRE(std::string(e.what()).find("before init") != std::string::npos);
    }
}

// ---------------------------------------------------------------------------
// (b) double init() throws std::logic_error
// ---------------------------------------------------------------------------
TEST_CASE("double init throws logic_error", "[simulation][lifecycle]")
{
    const fs::path ssp_dir = fixture_path("scenario");
    if (!fs::exists(ssp_dir / "SystemStructure.ssd"))
    {
        // No .ssp archive exists in the repo; without a real SSP fixture
        // init() cannot succeed and this test cannot be exercised.
        SKIP("requires an SSP fixture (resources/reference_ssp/artifacts/models/scenario/baseline)");
    }

    auto config = make_minimal_config();

    // Directory-based SSP: Archive ctor accepts a directory directly.
    ssp4cpp::Ssp ssp(ssp_dir);
    ssp4sim::Simulation sim(&ssp, &config);

    // First init() must succeed (happy path preserved).
    sim.init();

    // Second init() is a programming error -> must throw std::logic_error.
    REQUIRE_THROWS_AS(sim.init(), std::logic_error);
}

// ---------------------------------------------------------------------------
// (c) happy path: init() + simulate() completes
// ---------------------------------------------------------------------------
TEST_CASE("init then simulate completes", "[simulation][lifecycle]")
{
    const fs::path ssp_dir = fixture_path("scenario");
    if (!fs::exists(ssp_dir / "SystemStructure.ssd"))
    {
        SKIP("requires an SSP fixture (resources/reference_ssp/artifacts/models/scenario/baseline)");
    }

    auto config = make_minimal_config();
    config.start_time = 0;
    config.end_time = 1;
    config.timestep = 1;
    config.realtime = false;

    ssp4cpp::Ssp ssp(ssp_dir);
    ssp4sim::Simulation sim(&ssp, &config);

    sim.init();
    REQUIRE_NOTHROW(sim.simulate());
}