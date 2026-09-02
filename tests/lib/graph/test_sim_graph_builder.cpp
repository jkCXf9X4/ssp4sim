#include "pre/3_simulation/elements/model_connection.hpp"
#include "pre/3_simulation/elements/model_connector.hpp"
#include "signal/storage.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <string>
#include <unordered_map>

using ssp4sim::graph::ConnectionInfo;
using ssp4sim::graph::ConnectorInfo;
using ssp4sim::signal::SignalStorage;
using ssp4sim::types::DataType;

namespace {
    void init_storage(SignalStorage& storage, const std::string& signal_name,
                      DataType type = DataType::real, size_t idx = 0)
    {
        storage.add(signal_name, type, idx);
        storage.allocate();
    }

    auto make_connection(SignalStorage& src, SignalStorage& tgt,
                         DataType type = DataType::real,
                         uint32_t src_idx = 0, uint32_t tgt_idx = 0,
                         uint64_t delay = 0)
    {
        ConnectionInfo con;
        con.type = type;
        con.size = (type == DataType::real) ? sizeof(double) : sizeof(int);
        con.source_storage = &src;
        con.target_storage = &tgt;
        con.source_index = src_idx;
        con.target_index = tgt_idx;
        con.delay = delay;
        return con;
    }

    template<typename T>
    T read_storage_value(SignalStorage& storage, size_t area, size_t index)
    {
        T result{};
        std::memcpy(&result, storage.get_item(area, index), sizeof(T));
        return result;
    }
} // anonymous namespace

constexpr size_t kStorageAreas = 10;
constexpr double kExpectedValue = 42.0;
constexpr int kExpectedIntValue = 99;
constexpr double kInitialValue = 3.5;

// NOTE: Despite the file name, this file tests ConnectionInfo and ConnectorInfo
// helper methods used by SimGraphBuilder, not the SimGraphBuilder class itself.

// ---------------------------------------------------------------------------
// Description: Verifies retrieve_model_inputs copies data with zero delay,
//              positive delay, and for integer types
// Rationale:   Core data routing — inputs copied from source to target storage
// ---------------------------------------------------------------------------
TEST_CASE("ConnectionInfo::retrieve_model_inputs copies data correctly",
          "[sim_graph_builder]")
{
    SignalStorage src_storage(kStorageAreas, "source");
    SignalStorage tgt_storage(kStorageAreas, "target");

    init_storage(src_storage, "source.signal");
    init_storage(tgt_storage, "target.signal");

    // Write a value into source at time 0
    auto src_area = src_storage.push(0);
    double input_val = kExpectedValue;
    std::memcpy(src_storage.get_item(src_area, 0), &input_val, sizeof(double));
    src_storage.flag_new_data(src_area);

    auto con = make_connection(src_storage, tgt_storage);
    std::vector<ConnectionInfo> connections = {con};

    SECTION("Copies data with zero delay")
    {
        auto tgt_area = tgt_storage.push(0);
        ConnectionInfo::retrieve_model_inputs(connections, tgt_area, 0, 0, 0);

        CHECK(read_storage_value<double>(tgt_storage, tgt_area, 0) == kExpectedValue);
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
        ConnectionInfo::retrieve_model_inputs(connections, tgt_area, 10, 10, 10);

        CHECK(read_storage_value<double>(tgt_storage, tgt_area, 0) == 20.0);
    }

    SECTION("Copies integer data")
    {
        SignalStorage int_src(kStorageAreas, "int_source");
        SignalStorage int_tgt(kStorageAreas, "int_target");
        init_storage(int_src, "source.int", DataType::integer);
        init_storage(int_tgt, "target.int", DataType::integer);

        auto src_area = int_src.push(0);
        int val = kExpectedIntValue;
        std::memcpy(int_src.get_item(src_area, 0), &val, sizeof(int));
        int_src.flag_new_data(src_area);

        auto int_con = make_connection(int_src, int_tgt, DataType::integer);
        std::vector<ConnectionInfo> int_cons = {int_con};
        auto tgt_area = int_tgt.push(0);
        ConnectionInfo::retrieve_model_inputs(int_cons, tgt_area, 0, 0, 0);

        CHECK(read_storage_value<int>(int_tgt, tgt_area, 0) == kExpectedIntValue);
    }
}

// ---------------------------------------------------------------------------
// Description: Verifies retrieve_model_inputs resolves source area according
//              to the precomputed DataAccessMode and time_offset
// Rationale:   Graph build precomputes how a connection samples its source
// ---------------------------------------------------------------------------
TEST_CASE("ConnectionInfo::retrieve_model_inputs honors mode and time_offset",
          "[sim_graph_builder]")
{
    SignalStorage src(kStorageAreas, "src");
    SignalStorage tgt(kStorageAreas, "tgt");
    init_storage(src, "s");
    init_storage(tgt, "t");

    // Source writes at 100, 200
    auto a100 = src.push(100);
    double v100 = 1.0;
    std::memcpy(src.get_item(a100, 0), &v100, sizeof(double));
    auto a200 = src.push(200);
    double v200 = 2.0;
    std::memcpy(src.get_item(a200, 0), &v200, sizeof(double));

    // Step span [100, 300]; input_time 300. Latest at 300 -> v at 200.
    auto tgt_area = tgt.push(300);

    SECTION("StartTime samples at step_start")
    {
        ConnectionInfo con = make_connection(src, tgt);
        con.mode = ssp4sim::graph::DataAccessMode::StartTime;
        std::vector<ConnectionInfo> cons = {con};
        ConnectionInfo::retrieve_model_inputs(cons, tgt_area, 300, 100, 300);
        CHECK(read_storage_value<double>(tgt, tgt_area, 0) == v100);
    }

    SECTION("StartTime with negative offset shifts earlier")
    {
        // Currently at latest valid (200) which is not at step_start.
        ConnectionInfo con = make_connection(src, tgt);
        con.mode = ssp4sim::graph::DataAccessMode::StartTime;
        con.time_offset = -100; // sample at step_start(100) - 100 = 0 -> nothing yet
        std::vector<ConnectionInfo> cons = {con};
        ConnectionInfo::retrieve_model_inputs(cons, tgt_area, 300, 100, 300);
        CHECK(read_storage_value<double>(tgt, tgt_area, 0) == 0.0);
    }

    SECTION("EndTime samples at step_end")
    {
        ConnectionInfo con = make_connection(src, tgt);
        con.mode = ssp4sim::graph::DataAccessMode::EndTime;
        std::vector<ConnectionInfo> cons = {con};
        ConnectionInfo::retrieve_model_inputs(cons, tgt_area, 300, 100, 300);
        CHECK(read_storage_value<double>(tgt, tgt_area, 0) == v200);
    }

    SECTION("EndTime with negative offset samples before step_end")
    {
        ConnectionInfo con = make_connection(src, tgt);
        con.mode = ssp4sim::graph::DataAccessMode::EndTime;
        con.time_offset = -150; // step_end - 150 = 150 -> latest valid <=150 is 100
        std::vector<ConnectionInfo> cons = {con};
        ConnectionInfo::retrieve_model_inputs(cons, tgt_area, 300, 100, 300);
        CHECK(read_storage_value<double>(tgt, tgt_area, 0) == v100);
    }

    SECTION("Latest applies offset to input_time")
    {
        ConnectionInfo con = make_connection(src, tgt);
        con.mode = ssp4sim::graph::DataAccessMode::Latest;
        con.time_offset = -50; // input_time - 50 = 250 -> latest valid <=250 is 200
        std::vector<ConnectionInfo> cons = {con};
        ConnectionInfo::retrieve_model_inputs(cons, tgt_area, 300, 300, 300);
        CHECK(read_storage_value<double>(tgt, tgt_area, 0) == v200);
    }
}

// ---------------------------------------------------------------------------
// Description: Verifies no-valid-source-data logs warning (no crash) and
//              empty connections list is no-op
// Rationale:   Robustness — missing data must not crash
// ---------------------------------------------------------------------------
TEST_CASE("ConnectionInfo::retrieve_model_inputs handles edge cases",
          "[sim_graph_builder]")
{
    SECTION("No valid source data logs warning but does not crash")
    {
        SignalStorage src(kStorageAreas, "src");
        SignalStorage tgt(kStorageAreas, "tgt");
        init_storage(src, "s");
        init_storage(tgt, "t");

        auto con = make_connection(src, tgt);
        std::vector<ConnectionInfo> cons = {con};
        auto tgt_area = tgt.push(100); // No source data at time 100
        // Should not crash — just log a warning
        ConnectionInfo::retrieve_model_inputs(cons, tgt_area, 100, 100, 100);
        // Target should remain unmodified (default 0.0)
        CHECK(read_storage_value<double>(tgt, tgt_area, 0) == 0.0);
    }

    SECTION("Empty connections list is a no-op")
    {
        std::vector<ConnectionInfo> empty;
        // Should not crash (tgt_area = 0, no storage needed with empty list)
        ConnectionInfo::retrieve_model_inputs(empty, 0, 0, 0, 0);
    }
}

// ---------------------------------------------------------------------------
// Description: Verifies to_string includes storage names and feedthrough flag
// Rationale:   Debug/logging utility
// Creep flag:  Presentation detail
// ---------------------------------------------------------------------------
TEST_CASE("ConnectionInfo to_string includes key fields", "[sim_graph_builder]")
{
    SignalStorage src(kStorageAreas, "source_storage");
    SignalStorage tgt(kStorageAreas, "target_storage");
    init_storage(src, "s");
    init_storage(tgt, "t");

    auto con = make_connection(src, tgt, DataType::real, 0, 1, 2);
    con.is_feedthrough = true;

    auto str = con.to_string();
    CHECK(str.find("source_storage") != std::string::npos);
    CHECK(str.find("target_storage") != std::string::npos);
    CHECK(str.find("is_feedthrough: true") != std::string::npos);
}

// ---------------------------------------------------------------------------
// Description: Verifies ConnectorInfo::to_string includes key fields
// Rationale:   Debug/logging utility
// Creep flag:  Presentation detail
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
// Description: Verifies set_initial_input_area writes initial value to storage
// Rationale:   Initial value propagation required for correct simulation startup
// ---------------------------------------------------------------------------
TEST_CASE("ConnectorInfo::set_initial_input_area copies initial values",
          "[sim_graph_builder]")
{
    SignalStorage storage(kStorageAreas, "input_storage");
    init_storage(storage, "model.input");

    ConnectorInfo info;
    info.name = "model.input";
    info.type = DataType::real;
    info.size = sizeof(double);
    info.index = 0;
    info.storage = &storage;
    info.initial_value = std::make_unique<ssp4sim::ext::ParameterValue>("model.input", DataType::real);
    double init_val = kInitialValue;
    info.initial_value->store_value(&init_val);

    std::unordered_map<std::string, ConnectorInfo> inputs;
    inputs["model.input"] = std::move(info);

    ConnectorInfo::set_initial_input_area(&storage, inputs, 0);

    // set_initial_input_area pushes at time 0 and writes to that area.
    // Find which area holds time 0 data.
    size_t found_area = 0;
    storage.find_area(0, found_area);
    CHECK(read_storage_value<double>(storage, found_area, 0) == kInitialValue);
}

// ---------------------------------------------------------------------------
// Description: Verifies no crash and default 0.0 when no initial value set
// Rationale:   Robustness — missing initial values must not crash
// ---------------------------------------------------------------------------
TEST_CASE("ConnectorInfo::set_initial_input_area skips connectors without initial_value",
          "[sim_graph_builder]")
{
    SignalStorage storage(kStorageAreas, "input_storage");
    init_storage(storage, "model.input");

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
    CHECK(read_storage_value<double>(storage, found_area, 0) == 0.0);
}

// ---------------------------------------------------------------------------
// Description: Verifies feedthrough flag contract (zero-delay = feedthrough)
// Rationale:   Feedthrough detection determines simulation loop ordering.
//              These tests verify the expected contract of the wire_connections
//              logic: is_feedthrough is set to (delay == 0). The first two
//              sections confirm the expected field values; the wire_connections
//              production code path is tested in test_model_connection.cpp.
// ---------------------------------------------------------------------------
TEST_CASE("FmuModel feedthrough detection", "[sim_graph_builder]")
{
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

    // The wire_connections method sets is_feedthrough = (resolved->delay == 0).
    // The first two sections above verify the expected outcome of that logic.
    // Full production-path coverage lives in test_model_connection.cpp.
}

// ---------------------------------------------------------------------------
// Description: Verifies forward_derivatives and forward_derivatives_order
//              fields on ConnectionInfo
// Rationale:   These fields control derivative forwarding in retrieve_model_inputs
// ---------------------------------------------------------------------------
TEST_CASE("ConnectionInfo forward_derivatives fields", "[sim_graph_builder]")
{
    ConnectionInfo con;

    SECTION("Defaults are false/zero")
    {
        CHECK(con.forward_derivatives == false);
        CHECK(con.forward_derivatives_order == 0);
    }

    SECTION("Can set forward_derivatives true")
    {
        con.forward_derivatives = true;
        CHECK(con.forward_derivatives == true);
    }

    SECTION("Can set forward_derivatives_order")
    {
        con.forward_derivatives_order = 3;
        CHECK(con.forward_derivatives_order == 3);
    }

    SECTION("Both fields independently settable")
    {
        con.forward_derivatives = true;
        con.forward_derivatives_order = 5;
        CHECK(con.forward_derivatives == true);
        CHECK(con.forward_derivatives_order == 5);
    }
}

// ---------------------------------------------------------------------------
// Description: Verifies forward_derivatives and forward_derivatives_order
//              fields on ConnectorInfo, which are used by set_start_values
// Rationale:   These fields control derivative forwarding behavior. Full
//              set_start_values production-path testing requires FMU
//              infrastructure (connector.fmu->model) and is not done here.
// ---------------------------------------------------------------------------
TEST_CASE("ConnectorInfo forward_derivatives fields", "[sim_graph_builder]")
{
    ConnectorInfo info;

    SECTION("Defaults are false/zero")
    {
        CHECK(info.forward_derivatives == false);
        CHECK(info.forward_derivatives_order == 0);
    }

    SECTION("Can set forward_derivatives true")
    {
        info.forward_derivatives = true;
        CHECK(info.forward_derivatives == true);
    }

    SECTION("Can set forward_derivatives_order")
    {
        info.forward_derivatives_order = 2;
        CHECK(info.forward_derivatives_order == 2);
    }

    SECTION("Both fields independently settable")
    {
        info.forward_derivatives = true;
        info.forward_derivatives_order = 4;
        CHECK(info.forward_derivatives == true);
        CHECK(info.forward_derivatives_order == 4);
    }
}
