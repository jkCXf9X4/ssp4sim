#include "analysis/components/analysis_system.hpp"
#include "analysis/analysis_system_builder.hpp"
#include "analysis/components/analysis_model.hpp"
#include "analysis/components/analysis_connector.hpp"
#include "analysis/components/analysis_connection.hpp"

#include "handler/fmu_handler.hpp"

#include "ssp4cpp/ssp.hpp"

#include <catch2/catch_test_macros.hpp>

#include <filesystem>
#include <memory>
#include <set>
#include <string>

namespace fs = std::filesystem;

namespace
{
    fs::path project_root()
    {
        return fs::path(SSP4SIM_PROJECT_ROOT);
    }
}

TEST_CASE("Full pipeline: AnalysisSystemBuilder can build from path string", "[analysis_integration]")
{
    auto ssp_dir = project_root() / "resources" / "reference_ssp" / "artifacts" / "models" /
                   "pyfmu_csv_source_sink" / "baseline";
    auto sys = ssp4sim::analysis::AnalysisSystemBuilder().build(ssp_dir.string());
    REQUIRE(sys != nullptr);
    REQUIRE(sys->get_all_models().size() == 2);
}

TEST_CASE("Full pipeline: Builder produces models with FMU info", "[analysis_integration]")
{
    auto ssp_dir = project_root() / "resources" / "reference_ssp" / "artifacts" / "models" /
                   "pyfmu_csv_source_sink" / "baseline";
    auto sys = ssp4sim::analysis::AnalysisSystemBuilder().build(ssp_dir.string());

    // Verify models have FMU info
    for (auto *m : sys->get_all_models())
    {
        REQUIRE(m->fmu != nullptr);
        bool valid_name = (m->name == "source" || m->name == "sink");
        REQUIRE(valid_name);
    }

    // Verify connections exist
    auto connections = sys->get_all_connections();
    REQUIRE(connections.size() == 1);
    REQUIRE(connections[0]->source_model == "source");
    REQUIRE(connections[0]->target_model == "sink");
}

TEST_CASE("Full pipeline: Ssp-based builder produces same result as path-based", "[analysis_integration]")
{
    auto ssp_dir = project_root() / "resources" / "reference_ssp" / "artifacts" / "models" /
                   "pyfmu_csv_source_sink" / "baseline";

    auto ssp = std::make_unique<ssp4cpp::Ssp>(ssp_dir.string());
    auto fmu_handler = std::make_unique<ssp4sim::handler::FmuHandler>(ssp.get());
    fmu_handler->init();

    auto sys = ssp4sim::analysis::AnalysisSystemBuilder().build(ssp.get(), fmu_handler.get());
    REQUIRE(sys != nullptr);
    REQUIRE(sys->get_all_models().size() == 2);

    auto models = sys->get_all_models();
    std::set<std::string> names;
    for (auto *m : models)
        names.insert(m->name);
    REQUIRE(names.count("source") == 1);
    REQUIRE(names.count("sink") == 1);
}