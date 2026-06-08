#include "analysis/analysis_system.hpp"
#include "analysis/analysis_model.hpp"
#include "analysis/analysis_connector.hpp"
#include "analysis/analysis_connection.hpp"
#include "analysis/analysis_model_variable.hpp"

#include <catch2/catch_test_macros.hpp>

#include <memory>
#include <string>
#include <vector>

using ssp4sim::analysis::AnalysisSystem;
using ssp4sim::analysis::AnalysisModel;
using ssp4sim::analysis::AnalysisConnector;
using ssp4sim::analysis::AnalysisConnection;
using ssp4sim::analysis::AnalysisModelVariable;
using ssp4sim::types::Causality;
using ssp4sim::types::DataType;

// ---------------------------------------------------------------------------
// AnalysisSystem
// ---------------------------------------------------------------------------
TEST_CASE("AnalysisSystem construction with name", "[analysis_system]")
{
    AnalysisSystem sys("test-system");
    REQUIRE(sys.name == "test-system");
    REQUIRE(sys.models.empty());
    REQUIRE(sys.connectors.empty());
    REQUIRE(sys.connections.empty());
    REQUIRE(sys.nested_systems.empty());
}

TEST_CASE("AnalysisSystem get_all_models on empty system", "[analysis_system]")
{
    AnalysisSystem sys("empty");
    auto models = sys.get_all_models();
    REQUIRE(models.empty());
}

TEST_CASE("AnalysisSystem get_all_connections on empty system", "[analysis_system]")
{
    AnalysisSystem sys("empty");
    auto conns = sys.get_all_connections();
    REQUIRE(conns.empty());
}

TEST_CASE("AnalysisSystem detect_algebraic_loops on empty system", "[analysis_system]")
{
    AnalysisSystem sys("empty");
    auto loops = sys.detect_algebraic_loops();
    REQUIRE(loops.empty());
}

TEST_CASE("AnalysisSystem move construction", "[analysis_system]")
{
    AnalysisSystem sys("src");
    sys.models.push_back(std::make_unique<AnalysisModel>("m1", "", nullptr));
    REQUIRE(sys.models.size() == 1);

    AnalysisSystem dest(std::move(sys));
    REQUIRE(dest.name == "src");
    REQUIRE(dest.models.size() == 1);
    REQUIRE(sys.models.empty());
}

TEST_CASE("AnalysisSystem with one model and nested system", "[analysis_system]")
{
    AnalysisSystem root("root");
    root.models.push_back(std::make_unique<AnalysisModel>("m1", "file.fmu", nullptr));
    root.connections.push_back(std::make_unique<AnalysisConnection>("m1", "out", "m2", "in"));

    auto nested = std::make_unique<AnalysisSystem>("nested");
    nested->models.push_back(std::make_unique<AnalysisModel>("nested_m1", "nested.fmu", nullptr));
    root.nested_systems.push_back(std::move(nested));

    auto all_models = root.get_all_models();
    REQUIRE(all_models.size() == 2);
    REQUIRE(all_models[0]->name == "m1");
    REQUIRE(all_models[1]->name == "nested_m1");

    auto all_conns = root.get_all_connections();
    REQUIRE(all_conns.size() == 1);
    REQUIRE(all_conns[0]->source_model == "m1");
}

TEST_CASE("AnalysisSystem get_connector resolves dot-separated path", "[analysis_system]")
{
    AnalysisSystem sys("sys");
    auto conn = std::make_unique<AnalysisConnector>("comp", "conn1", 42, DataType::real);
    conn->name = "comp.conn1";
    sys.connectors.push_back(std::move(conn));

    auto *found = sys.get_connector("sys", "comp.conn1");
    REQUIRE(found != nullptr);
    REQUIRE(found->name == "comp.conn1");

    auto *not_found = sys.get_connector("sys", "nonexistent");
    REQUIRE(not_found == nullptr);
}

TEST_CASE("AnalysisSystem get_nested_system traverses hierarchy", "[analysis_system]")
{
    AnalysisSystem root("root");
    auto child = std::make_unique<AnalysisSystem>("child");
    auto grandchild = std::make_unique<AnalysisSystem>("grandchild");
    child->nested_systems.push_back(std::move(grandchild));
    root.nested_systems.push_back(std::move(child));

    auto *found = root.get_nested_system("child.grandchild");
    REQUIRE(found != nullptr);
    REQUIRE(found->name == "grandchild");

    auto *not_found = root.get_nested_system("root.child.missing");
    REQUIRE(not_found == nullptr);
}

// ---------------------------------------------------------------------------
// AnalysisModel
// ---------------------------------------------------------------------------
TEST_CASE("AnalysisModel construction and move", "[analysis_model]")
{
    AnalysisModel m("test_model", "test.fmu", nullptr);
    REQUIRE(m.name == "test_model");
    REQUIRE(m.source_file == "test.fmu");
    REQUIRE(m.fmu == nullptr);
    REQUIRE(m.connectors.empty());

    AnalysisModel m2(std::move(m));
    REQUIRE(m2.name == "test_model");
    REQUIRE(m.name.empty()); // moved-from state
}

TEST_CASE("AnalysisModel to_string includes key fields", "[analysis_model]")
{
    AnalysisModel m("model_a", "model_a.fmu", nullptr);
    auto s = m.to_string();
    REQUIRE(s.find("model_a") != std::string::npos);
    REQUIRE(s.find("model_a.fmu") != std::string::npos);
}

// ---------------------------------------------------------------------------
// AnalysisConnector
// ---------------------------------------------------------------------------
TEST_CASE("AnalysisConnector construction", "[analysis_connector]")
{
    auto c = std::make_unique<AnalysisConnector>("comp", "var", 100, DataType::real);
    REQUIRE(c->name == "comp.var");
    REQUIRE(c->value_reference == 100);
    REQUIRE(c->data_type == DataType::real);
    REQUIRE(c->is_boundary == false);
}

TEST_CASE("AnalysisConnector create_name utility", "[analysis_connector]")
{
    auto name = AnalysisConnector::create_name("motor", "torque");
    REQUIRE(name == "motor.torque");
}

TEST_CASE("AnalysisConnector boundary flag", "[analysis_connector]")
{
    AnalysisConnector c;
    c.name = "sys.U";
    c.is_boundary = true;
    REQUIRE(c.is_boundary == true);
    REQUIRE(c.name == "sys.U");
}

TEST_CASE("AnalysisConnector to_string", "[analysis_connector]")
{
    AnalysisConnector c;
    c.name = "sys.conn";
    auto s = c.to_string();
    REQUIRE(s.find("sys.conn") != std::string::npos);
}

// ---------------------------------------------------------------------------
// AnalysisConnection
// ---------------------------------------------------------------------------
TEST_CASE("AnalysisConnection construction from strings", "[analysis_connection]")
{
    AnalysisConnection conn("src_model", "src_con", "tgt_model", "tgt_con");
    REQUIRE(conn.source_model == "src_model");
    REQUIRE(conn.source_connector == "src_con");
    REQUIRE(conn.target_model == "tgt_model");
    REQUIRE(conn.target_connector == "tgt_con");
    REQUIRE(conn.is_boundary_crossing == false);
}

TEST_CASE("AnalysisConnection boundary crossing flag", "[analysis_connection]")
{
    AnalysisConnection conn("sys", "in", "sys", "out", 0, true);
    REQUIRE(conn.is_boundary_crossing == true);
}

TEST_CASE("AnalysisConnection create_name", "[analysis_connection]")
{
    auto name = AnalysisConnection::create_name("a", "b", "c", "d");
    REQUIRE(name == "a.b->c.d");
}

TEST_CASE("AnalysisConnection to_string", "[analysis_connection]")
{
    AnalysisConnection conn("src", "out", "tgt", "in");
    auto s = conn.to_string();
    REQUIRE(s.find("src") != std::string::npos);
    REQUIRE(s.find("tgt") != std::string::npos);
}

// ---------------------------------------------------------------------------
// AnalysisModelVariable
// ---------------------------------------------------------------------------
TEST_CASE("AnalysisModelVariable construction", "[analysis_model_variable]")
{
    AnalysisModelVariable mv("comp", "var", "Real", "42");
    REQUIRE(mv.component == "comp");
    REQUIRE(mv.variable_name == "var");
    REQUIRE(mv.type == "Real");
    REQUIRE(mv.name == "comp.var");
}

TEST_CASE("AnalysisModelVariable to_string", "[analysis_model_variable]")
{
    AnalysisModelVariable mv("comp", "x", "Real", "0");
    auto s = mv.to_string();
    REQUIRE(s.find("comp") != std::string::npos);
    REQUIRE(s.find("x") != std::string::npos);
}