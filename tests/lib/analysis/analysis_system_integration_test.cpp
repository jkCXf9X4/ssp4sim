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

TEST_CASE("Full pipeline: system connectors are all boundary, model connectors all non-boundary", "[analysis_integration][connector_placement]")
{
    auto ssp_dir = project_root() / "resources" / "reference_ssp" / "artifacts" / "models" /
                   "pyfmu_csv_source_sink" / "baseline";
    auto sys = ssp4sim::analysis::AnalysisSystemBuilder().build(ssp_dir.string());

    // System-level connectors should all be boundary
    for (auto &c : sys->connectors)
    {
        REQUIRE(c->is_boundary == true);
    }

    // Model-level connectors should all be non-boundary
    for (auto *m : sys->get_all_models())
    {
        for (auto &c : m->connectors)
        {
            REQUIRE(c->is_boundary == false);
        }
    }
}

TEST_CASE("Full pipeline: no is_boundary_crossing connections after construction", "[analysis_integration][boundary_crossing]")
{
    auto ssp_dir = project_root() / "resources" / "reference_ssp" / "artifacts" / "models" /
                   "pyfmu_csv_source_sink" / "baseline";
    auto sys = ssp4sim::analysis::AnalysisSystemBuilder().build(ssp_dir.string());

    // After construction, connections should have is_boundary_crossing reflecting
    // whether start/end element was missing in the SSP XML
    auto all_connections = sys->get_all_connections();
    for (auto *conn : all_connections)
    {
        // Flat SSP has no boundary crossings
        REQUIRE(conn->is_boundary_crossing == false);
    }
}

TEST_CASE("Full pipeline: nested system connector placement invariant", "[analysis_integration][nested]")
{
    auto ssp_dir = project_root() / "resources" / "reference_ssp" / "artifacts" / "models" /
                   "signal_nested_external_bindings" / "baseline";
    auto sys = ssp4sim::analysis::AnalysisSystemBuilder().build(ssp_dir.string());

    // Verify nested system structure
    REQUIRE(sys->nested_systems.size() == 1);
    REQUIRE(sys->nested_systems[0]->name == "inner");

    // System-level boundary connectors
    for (auto &c : sys->connectors)
    {
        REQUIRE(c->is_boundary == true);
    }
    for (auto &c : sys->nested_systems[0]->connectors)
    {
        REQUIRE(c->is_boundary == true);
    }

    // All model connectors are non-boundary
    for (auto *m : sys->get_all_models())
    {
        for (auto &c : m->connectors)
        {
            REQUIRE(c->is_boundary == false);
        }
    }
}