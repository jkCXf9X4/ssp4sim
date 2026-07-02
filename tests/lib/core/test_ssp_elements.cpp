#include "pre/1_ssp_parser/elements/ssp_connector.hpp"
#include "pre/1_ssp_parser/elements/ssp_connection.hpp"
#include "pre/1_ssp_parser/elements/_ssp_item.hpp"

#include <catch2/catch_test_macros.hpp>

#include <string>

using ssp4sim::analysis::SspConnector;
using ssp4sim::analysis::SspConnection;
using ssp4sim::analysis::SspItem;
using ssp4sim::analysis::SspItemType;
using ssp4sim::types::Causality;
using ssp4sim::types::DataType;

// ---------------------------------------------------------------------------
// Description: Verifies SspItem::to_string works via concrete derived type
// Rationale:   SspItem is abstract; tested through SspConnector
// Creep flag:  Presentation detail — to_string format may change
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// SspItem base class
// ---------------------------------------------------------------------------
TEST_CASE("SspItem base class", "[ssp_elements]")
{
    SECTION("to_string returns name with format placeholder")
    {
        // Test via a concrete derived type
        SspConnector conn("test.x", 0, DataType::real, Causality::input);
        auto str = conn.to_string();
        CHECK(str.find("test.x") != std::string::npos);
    }
}

// ---------------------------------------------------------------------------
// Description: Verifies SspConnector construction with all causality types
//              and data types, plus default empty dependencies
// Rationale:   Connectors are the fundamental unit of SSP signal topology
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// SspConnector construction and field access
// ---------------------------------------------------------------------------
TEST_CASE("SspConnector construction", "[ssp_elements]")
{
    SECTION("Input connector with real type")
    {
        SspConnector conn("model.u", 42, DataType::real, Causality::input);
        CHECK(conn.name == "model.u");
        CHECK(conn.type == SspItemType::Connector);
        CHECK(conn.value_reference == 42);
        CHECK(conn.data_type == DataType::real);
        CHECK(conn.causality == Causality::input);
        CHECK(conn.initial_value.name == "model.u");
        CHECK(conn.initial_value.type == DataType::real);
    }

    SECTION("Output connector with integer type")
    {
        SspConnector conn("model.y", 7, DataType::integer, Causality::output);
        CHECK(conn.name == "model.y");
        CHECK(conn.value_reference == 7);
        CHECK(conn.data_type == DataType::integer);
        CHECK(conn.causality == Causality::output);
    }

    SECTION("Parameter connector")
    {
        SspConnector conn("model.k", 99, DataType::real, Causality::parameter);
        CHECK(conn.name == "model.k");
        CHECK(conn.causality == Causality::parameter);
    }

    SECTION("Default dependencies is empty")
    {
        SspConnector conn("model.x", 0, DataType::real, Causality::input);
        CHECK(conn.dependencies.empty());
    }
}

// ---------------------------------------------------------------------------
// Description: Verifies SspConnector::to_string includes key fields
// Rationale:   Debug/logging utility
// Creep flag:  Presentation detail
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// SspConnector to_string
// ---------------------------------------------------------------------------
TEST_CASE("SspConnector to_string", "[ssp_elements]")
{
    SspConnector conn("model.u", 42, DataType::real, Causality::input);
    auto str = conn.to_string();
    CHECK(str.find("model.u") != std::string::npos);
    CHECK(str.find("42") != std::string::npos);
    CHECK(str.find("real") != std::string::npos);
    CHECK(str.find("input") != std::string::npos);
}

// ---------------------------------------------------------------------------
// Description: Verifies SspConnection construction for standard, boundary
//              (empty source/target), and non-boundary connections
// Rationale:   Boundary detection is critical for correct graph topology
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// SspConnection construction and field access
// ---------------------------------------------------------------------------
TEST_CASE("SspConnection construction", "[ssp_elements]")
{
    SECTION("Standard connection between two models")
    {
        SspConnection conn("sine", "y", "gain", "u");
        CHECK(conn.source_model == "sine");
        CHECK(conn.source_connector == "y");
        CHECK(conn.target_model == "gain");
        CHECK(conn.target_connector == "u");
        CHECK(conn.type == SspItemType::Connection);
        CHECK(conn.is_boundary == false);
        CHECK(conn.delay == 0);
        // Name is auto-generated
        CHECK(conn.name.find("sine.y->gain.u") != std::string::npos);
    }

    SECTION("Boundary connection (empty source model)")
    {
        SspConnection conn("", "sys_in", "model", "input");
        CHECK(conn.source_model == "");
        CHECK(conn.is_boundary == true);
    }

    SECTION("Boundary connection (empty target model)")
    {
        SspConnection conn("model", "output", "", "sys_out");
        CHECK(conn.target_model == "");
        CHECK(conn.is_boundary == true);
    }

    SECTION("Non-boundary connection has both models")
    {
        SspConnection conn("a", "out", "b", "in");
        CHECK(conn.is_boundary == false);
    }
}

// ---------------------------------------------------------------------------
// Description: Verifies set_custom sets delay (default 0, custom value)
// Rationale:   Delay is the primary mechanism for feedthrough control
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// SspConnection set_custom
// ---------------------------------------------------------------------------
TEST_CASE("SspConnection set_custom sets delay", "[ssp_elements]")
{
    SspConnection conn("src", "out", "tgt", "in");

    SECTION("Default delay is 0")
    {
        CHECK(conn.delay == 0);
    }

    SECTION("set_custom with delay")
    {
        conn.set_custom(5);
        CHECK(conn.delay == 5);
    }

    SECTION("set_custom with default delay")
    {
        conn.set_custom();
        CHECK(conn.delay == 0);
    }
}

// ---------------------------------------------------------------------------
// Description: Verifies SspConnection::to_string includes name and delay
// Rationale:   Debug/logging utility
// Creep flag:  Presentation detail
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// SspConnection to_string
// ---------------------------------------------------------------------------
TEST_CASE("SspConnection to_string", "[ssp_elements]")
{
    SspConnection conn("src", "out", "tgt", "in");
    conn.delay = 3;
    auto str = conn.to_string();
    CHECK(str.find("src.out->tgt.in") != std::string::npos);
    CHECK(str.find("3") != std::string::npos);
}

// ---------------------------------------------------------------------------
// Description: Verifies SspItemType::to_string produces readable names
// Rationale:   Debug/logging utility
// Creep flag:  Presentation detail
// ---------------------------------------------------------------------------
// ---------------------------------------------------------------------------
// SspItemType to_string
// ---------------------------------------------------------------------------
TEST_CASE("SspItemType to_string produces readable names", "[ssp_elements]")
{
    CHECK(ssp4sim::analysis::to_string(SspItemType::Connector) == "Connector");
    CHECK(ssp4sim::analysis::to_string(SspItemType::Connection) == "Connection");
    CHECK(ssp4sim::analysis::to_string(SspItemType::ModelVariable) == "ModelVariable");
    CHECK(ssp4sim::analysis::to_string(SspItemType::Model) == "Model");
    CHECK(ssp4sim::analysis::to_string(SspItemType::System) == "System");
}