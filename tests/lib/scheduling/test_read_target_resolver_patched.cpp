#include "resolver/read_resolver.hpp"
#include "resolver/read_target_core.hpp"

#include "pre/3_simulation_graph/elements/model_fmu.hpp"
#include "pre/3_simulation_graph/elements/model_connection.hpp"

#include "utils/config.hpp"

#include "signal/storage.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

using ssp4sim::scheduling::DataAccessResolver;
using ssp4sim::scheduling::AccessMode;
using ssp4sim::graph::FmuModel;
using ssp4sim::graph::ConnectionInfo;
using ssp4sim::graph::Invocable;
using ssp4sim::signal::SignalStorage;
using ssp4sim::types::DataType;

// ---------------------------------------------------------------------------
// Description: the resolver -> FmuModel::pre() wiring copies a string (D15)
//              field from a committed producer's output area into the consumer's
//              input area, through the real FmuModel::pre() path.
// Rationale:   Drives the real production path: FmuModel::pre() resolves each
//              connection through the shared DataAccessResolver and runs the
//              type-aware copy (detail::copy_connection, string branch).
//              No FMU is required: FmuModel's constructor only moves the (null)
//              FmuInfo, pre() never dereferences fmu->model, and the destructor
//              null-checks before terminate().
// ---------------------------------------------------------------------------
TEST_CASE("resolver copies a string field through FmuModel::pre", "[ReadResolver][D15]")
{
    // FmuModel's constructor reads Config::getOr(...) which throws when no
    // config is loaded; load a minimal config first (mirrors test_config.cpp).
    ssp4sim::utils::Config::loadFromString(R"json({
        "simulation": {
            "executor": { "forward_derivatives": false },
            "log": { "fmu": false }
        }
    })json");

    // Producer: owns the source output storage that the consumer reads from.
    FmuModel producer("producer", nullptr, 0);
    // Consumer: owns the target input storage that pre() writes to.
    FmuModel consumer("consumer", nullptr, 0);

    // One string variable in each storage (D15: string-object copy, not memcpy).
    const auto src_index = producer.output_area->add_variable("producer.out", DataType::string, 0);
    const auto tgt_index = consumer.input_area->add_variable("consumer.in", DataType::string, 0);
    producer.output_area->allocate();
    consumer.input_area->allocate();

    // Wire producer.output -> consumer.input as a zero-delay connection.
    ConnectionInfo con;
    con.type = DataType::string;
    con.size = sizeof(std::string);
    con.source_storage = producer.output_area.get();
    con.target_storage = consumer.input_area.get();
    con.source_index = src_index;
    con.target_index = tgt_index;
    con.delay = 0;
    consumer.connections.push_back(con);

    // Build the resolver over the graph (producer + consumer), keyed by the
    // models' Invocable ids. The consumer's connection is index-aligned with
    // the resolver's internal edge table.
    std::vector<Invocable *> models{&producer, &consumer};
    std::shared_ptr<DataAccessResolver> resolver =
        std::make_shared<DataAccessResolver>(models);
    consumer.access_resolver = resolver;

    // Producer commits a string value at t=100 (D17: mark_committed publishes
    // the frontier after the value bytes are visible).
    const auto src_area = producer.output_area->push(100);
    *(std::string *)producer.output_area->get_item(src_area, src_index) = "hello-d15";
    resolver->mark_committed(producer.id, 100, src_area);

    // Consumer's pre() copies the committed string into its input area.
    consumer.pre(/*step_start=*/100, /*step_end=*/200);

    // The copy must be a real string copy (D15), not a byte copy.
    std::size_t tgt_area = 0;
    REQUIRE(consumer.input_area->find_area(100, tgt_area));
    REQUIRE(*(std::string *)consumer.input_area->get_item(tgt_area, tgt_index) == "hello-d15");
}