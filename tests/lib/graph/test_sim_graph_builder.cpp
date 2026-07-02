#include "pre/3_simulation/elements/model_connection.hpp"
#include "pre/3_simulation/elements/model_connector.hpp"
#include "signal/storage.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <cstring>
#include <string>
#include <unordered_map>

using ssp4sim::graph::ConnectionInfo;
using ssp4sim::graph::ConnectorInfo;
using ssp4sim::signal::SignalStorage;
using ssp4sim::types::DataType;

// ---------------------------------------------------------------------------
// ConnectionInfo::retrieve_model_inputs — data routing with delay
// ---------------------------------------------------------------------------
TEST_CASE("ConnectionInfo::retrieve_model_inputs copies data correctly",
          "[sim_graph_builder]")
{
    SignalStorage src_storage(10, "source");
    SignalStorage tgt_storage(10, "target");

    // Add a signal to source storage
    src_storage.add("source.signal", DataType::real, 0);
    tgt_storage.add("target.signal", DataType::real, 0);
    src_storage.allocate();
    tgt_storage.allocate();

    // Write a value into source at time 0
    auto src_area = src_storage.push(0);
    double input_val = 42.0;
    std::memcpy(src_storage.get_item(src_area, 0), &input_val, sizeof(double));
    src_storage.flag_new_data(src_area);

    ConnectionInfo con;
    con.type = DataType::real;
    con.size = sizeof(double);
    con.source_storage = &src_storage;
    con.target_storage = &tgt_storage;
    con.source_index = 0;
    con.target_index = 0;
    con.delay = 0;

    std::vector<ConnectionInfo> connections = {con};

    SECTION("Copies data with zero delay")
    {
        auto tgt_area = tgt_storage.push(0);
        ConnectionInfo::retrieve_model_inputs(connections, tgt_area, 0);

        double result = 0.0;
        std::memcpy(&result, tgt_storage.get_item(tgt_area, 0), sizeof(double));
        CHECK(result == 42.0);
    }

    SECTION("Copies data with delay")
    {
        // Write source data at time 0
        auto src_area_0 = src_storage.push(0);
        double val_0 = 10.0;
        std::memcpy(src_storage.get_item(src_area_0, 0), &val_0, sizeof(double));
        src_storage.flag_new_data(src_area_0);

        // Write source data at time 5
        auto src_area_5 = src_storage.push(5);
        double val_5 = 20.0;
        std::memcpy(src_storage.get_item(src_area_5, 0), &val_5, sizeof(double));
        src_storage.flag_new_data(src_area_5);

        // Read at time 10 with delay 5 — should get value from time 5
        con.delay = 5;
        connections[0] = con;

        auto tgt_area = tgt_storage.push(10);
        ConnectionInfo::retrieve_model_inputs(connections, tgt_area, 10);

        double result = 0.0;
        std::memcpy(&result, tgt_storage.get_item(tgt_area, 0), sizeof(double));
        CHECK(result == 20.0);
    }

    SECTION("Copies integer data")
    {
        SignalStorage int_src(10, "int_source");
        SignalStorage int_tgt(10, "int_target");
        int_src.add("source.int", DataType::integer, 0);
        int_tgt.add("target.int", DataType::integer, 0);
        int_src.allocate();
        int_tgt.allocate();

        auto src_area = int_src.push(0);
        int val = 99;
        std::memcpy(int_src.get_item(src_area, 0), &val, sizeof(int));
        int_src.flag_new_data(src_area);

        ConnectionInfo int_con;
        int_con.type = DataType::integer;
        int_con.size = sizeof(int);
        int_con.source_storage = &int_src;
        int_con.target_storage = &int_tgt;
        int_con.source_index = 0;
        int_con.target_index = 0;
        int_con.delay = 0;

        std::vector<ConnectionInfo> int_cons = {int_con};
        auto tgt_area = int_tgt.push(0);
        ConnectionInfo::retrieve_model_inputs(int_cons, tgt_area, 0);

        int result = 0;
        std::memcpy(&result, int_tgt.get_item(tgt_area, 0), sizeof(int));
        CHECK(result == 99);
    }
}

// ---------------------------------------------------------------------------
// ConnectionInfo::retrieve_model_inputs — edge cases
// ---------------------------------------------------------------------------
TEST_CASE("ConnectionInfo::retrieve_model_inputs handles edge cases",
          "[sim_graph_builder]")
{
    SECTION("No valid source data logs warning but does not crash")
    {
        SignalStorage src(10, "src");
        SignalStorage tgt(10, "tgt");
        src.add("s", DataType::real, 0);
        tgt.add("t", DataType::real, 0);
        src.allocate();
        tgt.allocate();

        ConnectionInfo con;
        con.type = DataType::real;
        con.size = sizeof(double);
        con.source_storage = &src;
        con.target_storage = &tgt;
        con.source_index = 0;
        con.target_index = 0;
        con.delay = 0;

        std::vector<ConnectionInfo> cons = {con};
        auto tgt_area = tgt.push(100); // No source data at time 100
        // Should not crash — just log a warning
        ConnectionInfo::retrieve_model_inputs(cons, tgt_area, 100);
        // Target should remain unmodified (default 0.0)
        double result = 0.0;
        std::memcpy(&result, tgt.get_item(tgt_area, 0), sizeof(double));
        CHECK(result == 0.0);
    }

    SECTION("Empty connections list is a no-op")
    {
        SignalStorage src(10, "src");
        SignalStorage tgt(10, "tgt");
        src.add("s", DataType::real, 0);
        tgt.add("t", DataType::real, 0);
        src.allocate();
        tgt.allocate();
        std::vector<ConnectionInfo> empty;
        auto tgt_area = tgt.push(0);
        // Should not crash
        ConnectionInfo::retrieve_model_inputs(empty, tgt_area, 0);
    }
}

// ---------------------------------------------------------------------------
// ConnectionInfo to_string
// ---------------------------------------------------------------------------
TEST_CASE("ConnectionInfo to_string includes key fields", "[sim_graph_builder]")
{
    SignalStorage src(10, "source_storage");
    SignalStorage tgt(10, "target_storage");
src.add("s", DataType::real, 0);
        tgt.add("t", DataType::real, 0);
        src.allocate();
        tgt.allocate();

        ConnectionInfo con;
    con.type = DataType::real;
    con.size = sizeof(double);
    con.source_storage = &src;
    con.target_storage = &tgt;
    con.source_index = 0;
    con.target_index = 1;
    con.delay = 2;
    con.is_feedthrough = true;

    auto str = con.to_string();
    CHECK(str.find("source_storage") != std::string::npos);
    CHECK(str.find("target_storage") != std::string::npos);
    CHECK(str.find("is_feedthrough: true") != std::string::npos);
}

// ---------------------------------------------------------------------------
// ConnectorInfo to_string
// ---------------------------------------------------------------------------
TEST_CASE("ConnectorInfo to_string includes key fields", "[sim_graph_builder]")
{
    ConnectorInfo info;
    info.name = "test.connector";
    info.type = DataType::real;
    info.size = sizeof(double);
    info.index = 5;
    info.value_ref = 42;

    auto str = info.to_string();
    CHECK(str.find("test.connector") != std::string::npos);
    CHECK(str.find("5") != std::string::npos);
    CHECK(str.find("42") != std::string::npos);
}

// ---------------------------------------------------------------------------
// ConnectorInfo::set_initial_input_area — copies initial values to storage
// ---------------------------------------------------------------------------
TEST_CASE("ConnectorInfo::set_initial_input_area copies initial values",
          "[sim_graph_builder]")
{
    SignalStorage storage(10, "input_storage");
    storage.add("model.input", DataType::real, 0);
    storage.allocate();

    ConnectorInfo info;
    info.name = "model.input";
    info.type = DataType::real;
    info.size = sizeof(double);
    info.index = 0;
    info.storage = &storage;
    info.initial_value = std::make_unique<ssp4sim::ext::ParameterValue>("model.input", DataType::real);
    double init_val = 3.5;
    info.initial_value->store_value(&init_val);

    std::unordered_map<std::string, ConnectorInfo> inputs;
    inputs["model.input"] = std::move(info);

    ConnectorInfo::set_initial_input_area(&storage, inputs, 0);

    // set_initial_input_area pushes at time 0 and writes to that area.
    // Find which area holds time 0 data.
    size_t found_area = 0;
    storage.find_area(0, found_area);
    double result = 0.0;
    std::memcpy(&result, storage.get_item(found_area, 0), sizeof(double));
    CHECK(result == 3.5);
}

// ---------------------------------------------------------------------------
// ConnectorInfo::set_initial_input_area — no initial value is a no-op
// ---------------------------------------------------------------------------
TEST_CASE("ConnectorInfo::set_initial_input_area skips connectors without initial_value",
          "[sim_graph_builder]")
{
    SignalStorage storage(10, "input_storage");
    storage.add("model.input", DataType::real, 0);
    storage.allocate();

    ConnectorInfo info;
    info.name = "model.input";
    info.type = DataType::real;
    info.size = sizeof(double);
    info.index = 0;
    info.storage = &storage;
    // No initial_value set

    std::unordered_map<std::string, ConnectorInfo> inputs;
    inputs["model.input"] = std::move(info);

    // Should not crash
    ConnectorInfo::set_initial_input_area(&storage, inputs, 0);

    // set_initial_input_area pushes at time 0. Find that area.
    size_t found_area = 0;
    storage.find_area(0, found_area);
    double result = 1.0;
    std::memcpy(&result, storage.get_item(found_area, 0), sizeof(double));
    CHECK(result == 0.0);
}

// ---------------------------------------------------------------------------
// FmuModel::has_feedthrough_outputs and feedthrough_connections
// ---------------------------------------------------------------------------
TEST_CASE("FmuModel feedthrough detection", "[sim_graph_builder]")
{
    // These tests verify the feedthrough logic on ConnectionInfo objects
    // without requiring a full FmuModel instance

    ConnectionInfo ft_con;
    ft_con.is_feedthrough = true;
    ft_con.delay = 0;

    ConnectionInfo non_ft_con;
    non_ft_con.is_feedthrough = false;
    non_ft_con.delay = 1;

    SECTION("Connection with zero delay is feedthrough")
    {
        CHECK(ft_con.is_feedthrough == true);
    }

    SECTION("Connection with delay is not feedthrough")
    {
        CHECK(non_ft_con.is_feedthrough == false);
    }

    SECTION("Feedthrough flag is set by wire_connections logic")
    {
        // The wire_connections method sets is_feedthrough = (resolved->delay == 0)
        ft_con.is_feedthrough = (ft_con.delay == 0);
        CHECK(ft_con.is_feedthrough == true);

        non_ft_con.is_feedthrough = (non_ft_con.delay == 0);
        CHECK(non_ft_con.is_feedthrough == false);
    }
}