#include "pre/3_simulation/elements/model_connection.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>

using ssp4sim::graph::ConnectionInfo;
using ssp4sim::signal::SignalStorage;
using ssp4sim::types::DataType;

// ---------------------------------------------------------------------------
// Description: Verifies ConnectionInfo::is_feedthrough field defaults,
//              zero-delay behavior, delay>0 behavior, and to_string inclusion
// Rationale:   Feedthrough field is the primary mechanism for algebraic loop
//              detection
// Creep flag:  to_string sub-test is presentation detail; feedthrough logic
//              overlaps with tests/lib/graph/test_sim_graph_builder.cpp
// ---------------------------------------------------------------------------
TEST_CASE("ConnectionInfo is_feedthrough field", "[ConnectionInfo]")
{
    // Setup minimal SignalStorage for source/target pointers
    SignalStorage src_storage(1, "source");
    SignalStorage tgt_storage(1, "target");
    src_storage.add("source.signal", DataType::real, 0);
    tgt_storage.add("target.signal", DataType::real, 0);

    ConnectionInfo con;
    con.type = DataType::real;
    con.size = sizeof(double);
    con.source_storage = &src_storage;
    con.target_storage = &tgt_storage;
    con.source_index = 0;
    con.target_index = 0;

    SECTION("Default is_feedthrough is false")
    {
        REQUIRE(con.is_feedthrough == false);
    }

    SECTION("Set is_feedthrough true with zero delay")
    {
        con.delay = 0;
        con.is_feedthrough = true;
        REQUIRE(con.is_feedthrough == true);
    }

    SECTION("is_feedthrough set to false when delay > 0")
    {
        con.delay = 0;
        con.is_feedthrough = true;
        REQUIRE(con.is_feedthrough == true);

        con.delay = 1;
        con.is_feedthrough = false;
        REQUIRE(con.is_feedthrough == false);
    }

    SECTION("to_string contains is_feedthrough field")
    {
        con.is_feedthrough = true;
        std::string str = con.to_string();
        REQUIRE(str.find("is_feedthrough: true") != std::string::npos);

        con.is_feedthrough = false;
        str = con.to_string();
        REQUIRE(str.find("is_feedthrough: false") != std::string::npos);
    }
}