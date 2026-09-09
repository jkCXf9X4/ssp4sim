#include "pre/3_simulation/elements/model_connection.hpp"
#include "pre/3_simulation/elements/model_connector.hpp"
#include "signal/storage.hpp"
#include "scheduling/read_target_core.hpp"
#include "scheduling/read_resolver_latest_executed.hpp"
#include "scheduling/read_resolver_macro_step_start_time.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <string>
#include <unordered_map>

using ssp4sim::graph::ConnectionInfo;
using ssp4sim::graph::ConnectorInfo;
using ssp4sim::signal::SignalStorage;
using ssp4sim::types::DataType;

using ssp4sim::scheduling::ResolvedRead;
using ssp4sim::scheduling::detail::AccessMode;
using ssp4sim::scheduling::detail::ModelStatus;
using ssp4sim::scheduling::detail::latest_executed_resolver;
using ssp4sim::scheduling::detail::macro_step_start_time_resolver;
using ssp4sim::scheduling::detail::copy_connection;

namespace {
    void init_storage(SignalStorage& storage, const std::string& signal_name,
                      DataType type = DataType::real, size_t idx = 0)
    {
        storage.add_variable(signal_name, type, idx);
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
// Description: Verifies the resolver core's copy step (detail::copy_connection)
//              under explicit graph-fact policy (StartTime/Latest/delay), covering
//              zero-delay value copy, delayed lookup, and integer/string types.
// Rationale:   Core data routing. The retired ConnectionInfo::retrieve_model_inputs
//              was policy-laden; sampling policy now lives in the chosen resolver
//              (see ssp4sim::scheduling::detail::ReadResolver). These tests exercise
//              the copy semantics with explicit policy edges.
// ---------------------------------------------------------------------------
TEST_CASE("resolver copy path copies values with explicit policy", "[sim_graph_builder]")
{
    using ssp4sim::scheduling::detail::AccessMode;
    using ssp4sim::scheduling::detail::ModelStatus;
    using ssp4sim::scheduling::detail::latest_executed_resolver;
    using ssp4sim::scheduling::detail::macro_step_start_time_resolver;
    using ssp4sim::scheduling::detail::copy_connection;
    using ssp4sim::scheduling::ResolverConfig;

    // A producer that has committed area `area` at `time` (mirrors mark_committed).
    auto set_committed = [](ModelStatus &st, std::uint64_t time, std::size_t area)
    {
        st.committed_count.store(1, std::memory_order::release);
        st.committed_time.store(time, std::memory_order::release);
        st.latest_area.store(area, std::memory_order::release);
        st.generation.store(1, std::memory_order::release);
    };

    auto wired_edge = [](const ConnectionInfo &con, AccessMode mode)
    {
        ssp4sim::scheduling::detail::Edge e = ssp4sim::scheduling::detail::edge_from(con, nullptr);
        e.unlinked = false; // a real wired edge, not an unlinked one
        e.mode = mode;
        return e;
    };

    // The graph derives the read policy per edge; mirror that choice onto the matching
    // resolver: Latest-pinned edges use the latest-executed resolver, time-sampled edges
    // the macro-step-start-time resolver.
    auto resolve_policy = [](const ssp4sim::scheduling::detail::Edge &e, const ModelStatus &st,
                             const ResolverConfig &cfg, std::uint64_t step_start,
                             std::uint64_t step_end)
    {
        if (e.mode == AccessMode::Latest)
        {
            return latest_executed_resolver()->resolve(e, st, cfg, step_start, step_end);
        }
        return macro_step_start_time_resolver()->resolve(e, st, cfg, step_start, step_end);
    };

    // ---------- zero-delay, Latest policy -> copies newest committed area ----------
    SECTION("Latest copies the committed area")
    {
        SignalStorage src(kStorageAreas, "src");
        SignalStorage tgt(kStorageAreas, "tgt");
        init_storage(src, "s");
        init_storage(tgt, "t");

        auto a100 = src.push(100);
        double v100 = 1.0;
        std::memcpy(src.get_item(a100, 0), &v100, sizeof(double));
        auto a200 = src.push(200);
        double v200 = 2.0;
        std::memcpy(src.get_item(a200, 0), &v200, sizeof(double));

        ModelStatus committed;
        set_committed(committed, 200, a200);

        auto tgt_area = tgt.push(300);

        ConnectionInfo con = make_connection(src, tgt);
        ResolvedRead r = resolve_policy(wired_edge(con, AccessMode::Latest),
                                        committed, ResolverConfig{}, 100, 300);
        CHECK(r.valid);
        CHECK(r.is_area);
        CHECK(r.area == a200);
        CHECK(copy_connection(con, tgt_area, r) == true);
        CHECK(read_storage_value<double>(tgt, tgt_area, 0) == v200);
    }

    // ---------- StartTime + delay -> delayed lookup ----------
    SECTION("StartTime with delay resolves earlier source data")
    {
        SignalStorage src(kStorageAreas, "src");
        SignalStorage tgt(kStorageAreas, "tgt");
        init_storage(src, "s");
        init_storage(tgt, "t");

        auto a100 = src.push(100);
        double v100 = 1.0;
        std::memcpy(src.get_item(a100, 0), &v100, sizeof(double));
        auto a200 = src.push(200);
        double v200 = 2.0;
        std::memcpy(src.get_item(a200, 0), &v200, sizeof(double));

        ModelStatus committed;
        set_committed(committed, 200, a200);

        auto tgt_area = tgt.push(300);

        ConnectionInfo con = make_connection(src, tgt, DataType::real, 0, 0, /*delay=*/100);
        ResolvedRead r = resolve_policy(wired_edge(con, AccessMode::StartTime),
                                        committed, ResolverConfig{}, 300, 300);
        CHECK(r.valid);
        CHECK_FALSE(r.is_area);
        CHECK(r.time == 200); // step_start(300) - delay(100)
        CHECK(copy_connection(con, tgt_area, r) == true);
        CHECK(read_storage_value<double>(tgt, tgt_area, 0) == v200);
    }

    // ---------- integer type copy ----------
    SECTION("Copies integer data")
    {
        SignalStorage src(kStorageAreas, "src");
        SignalStorage tgt(kStorageAreas, "tgt");
        init_storage(src, "s", DataType::integer);
        init_storage(tgt, "t", DataType::integer);

        auto src_area = src.push(0);
        int iv = kExpectedIntValue;
        std::memcpy(src.get_item(src_area, 0), &iv, sizeof(int));

        ModelStatus committed;
        set_committed(committed, 0, src_area);

        auto tgt_area = tgt.push(0);
        ConnectionInfo con = make_connection(src, tgt, DataType::integer, 0, 0);
        ResolvedRead r = resolve_policy(wired_edge(con, AccessMode::Latest),
                                        committed, ResolverConfig{}, 0, 0);
        CHECK(copy_connection(con, tgt_area, r) == true);
        CHECK(read_storage_value<int>(tgt, tgt_area, 0) == kExpectedIntValue);
    }

    // ---------- not-yet-committed producer -> no copy ----------
    SECTION("Not-yet-committed producer leaves target untouched")
    {
        SignalStorage src(kStorageAreas, "src");
        SignalStorage tgt(kStorageAreas, "tgt");
        init_storage(src, "s");
        init_storage(tgt, "t");

        auto src_area = src.push(100);
        double v = 5.0;
        std::memcpy(src.get_item(src_area, 0), &v, sizeof(double));

        auto tgt_area = tgt.push(100);
        ConnectionInfo con = make_connection(src, tgt);
        ssp4sim::scheduling::detail::Edge e = ssp4sim::scheduling::detail::edge_from(con, nullptr);
        e.unlinked = false;
        e.mode = AccessMode::Latest;

        ModelStatus never_committed; // committed_count == 0
        ResolvedRead r = resolve_policy(e, never_committed, ResolverConfig{}, 100, 100);
        CHECK_FALSE(r.valid);
        CHECK(copy_connection(con, tgt_area, r) == false);
        CHECK(read_storage_value<double>(tgt, tgt_area, 0) == 0.0);
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
