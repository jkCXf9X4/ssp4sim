#include <catch2/catch_test_macros.hpp>

#include "sim_graph_builder.hpp"

#include "signal/recorder.hpp"

using ssp4sim::signal::DataRecorder;

// ---------------------------------------------------------------------------
// GraphBuilder construction
// ---------------------------------------------------------------------------
TEST_CASE("GraphBuilder constructs with record_inputs flag", "[sim_graph_builder]")
{
    SECTION("record_inputs = false")
    {
        ssp4sim::graph::GraphBuilder builder(false);
        REQUIRE_FALSE(builder.record_inputs);
    }

    SECTION("record_inputs = true")
    {
        ssp4sim::graph::GraphBuilder builder(true);
        REQUIRE(builder.record_inputs);
    }
}

// ---------------------------------------------------------------------------
// register_model_storages — null-safety
// ---------------------------------------------------------------------------
TEST_CASE("register_model_storages handles null recorder", "[sim_graph_builder]")
{
    std::map<std::string, std::unique_ptr<ssp4sim::graph::Invocable>> empty_models;
    REQUIRE_NOTHROW(ssp4sim::graph::register_model_storages(empty_models, nullptr));
}

TEST_CASE("register_model_storages skips non-FmuModel entries", "[sim_graph_builder]")
{
    DataRecorder recorder(false);

    std::map<std::string, std::unique_ptr<ssp4sim::graph::Invocable>> models;
    models["not_an_fmu"] = nullptr;

    // Should not crash — the loop checks for null model.get() via dynamic_cast
    REQUIRE_NOTHROW(ssp4sim::graph::register_model_storages(models, &recorder));
}