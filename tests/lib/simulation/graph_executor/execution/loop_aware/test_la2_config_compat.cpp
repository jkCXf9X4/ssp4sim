// Config-boundary tests for the executor configuration parser + the la2
// strategy (patch task P1: findings A, B-keys, C, E).
//
// Verifies:
// (a) the centralized parser, `ssp4sim::ExecutorOptions::load()` (called from
//     SharedConfig), reads `simulation.executor.*` and ignores the legacy
//     `simulation.executor.loop_aware.*` namespace;
// (b) `ExecutorBuilder` accepts both `la2` and the legacy `loop_aware` method
//     names and dispatches both to the la2 strategy's SerialSeidel stack,
//     wrapped in a MacroExecutor;
// (c) legacy mode values "fixed" / "geometric" map to the same behavior as
//     "linear" / "factor";
// (d) requesting "parallel_seidel" throws a clear std::runtime_error at
//     dispatch time.
//
// Behavior is verified through `ExecutorBuilder` over typed `ExecutorOptions`
// (the parser is exercised independently in (a)); no executor reads the global
// utils::Config.
//
// Executors share ownership of their nodes (std::shared_ptr), so the test
// builds shared graphs and hands a shared copy to each executor; a raw-pointer
// snapshot is kept for invocation counting.

#include <catch2/catch_test_macros.hpp>

#include "config.hpp"
#include "executor/seidel/seidel_serial.hpp"
#include "executor_builder.hpp"
#include "executor/substep/macro_substep_executor.hpp"
#include "shared_config.hpp"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

using ssp4sim::utils::Config;

namespace
{
    // Minimal Invocable that counts how many times it was invoked.
    class MockNode final : public ssp4sim::graph::Invocable
    {
    public:
        explicit MockNode(std::string name)
        {
            this->name = std::move(name);
        }

        uint64_t invoke(ssp4sim::graph::StepData) override
        {
            ++invoke_count;
            return 0;
        }

        std::size_t invoke_count = 0;
    };

    // Raw-pointer snapshot of the shared storage, kept for counting after the
    // storage is handed to an executor.
    std::vector<ssp4sim::graph::Invocable *> snapshot(
        const std::vector<std::shared_ptr<MockNode>> &storage)
    {
        std::vector<ssp4sim::graph::Invocable *> out;
        out.reserve(storage.size());
        for (const auto &n : storage)
        {
            out.push_back(n.get());
        }
        return out;
    }

    // Copy the shared storage into a shared Invocable vector for an executor.
    std::vector<std::shared_ptr<ssp4sim::graph::Invocable>> to_owned(
        const std::vector<std::shared_ptr<MockNode>> &storage)
    {
        return std::vector<std::shared_ptr<ssp4sim::graph::Invocable>>(
            storage.begin(), storage.end());
    }

    // A -> B -> A: one 2-node loop SCC (the only SCC).
    std::vector<ssp4sim::graph::Invocable *> make_loop_graph(
        std::vector<std::shared_ptr<MockNode>> &storage)
    {
        storage.clear();
        storage.push_back(std::make_shared<MockNode>("A"));
        storage.push_back(std::make_shared<MockNode>("B"));
        auto *a = storage[0].get();
        auto *b = storage[1].get();
        a->add_child(b);
        b->add_child(a);
        return snapshot(storage);
    }

    // A -> B -> C: three single-node SCCs (no loops).
    std::vector<ssp4sim::graph::Invocable *> make_chain_graph(
        std::vector<std::shared_ptr<MockNode>> &storage)
    {
        storage.clear();
        storage.push_back(std::make_shared<MockNode>("A"));
        storage.push_back(std::make_shared<MockNode>("B"));
        storage.push_back(std::make_shared<MockNode>("C"));
        auto *a = storage[0].get();
        auto *b = storage[1].get();
        auto *c = storage[2].get();
        a->add_child(b);
        b->add_child(c);
        return snapshot(storage);
    }

    // Total invocations across all nodes (raw snapshot of the owned graph).
    std::size_t total_invocations(const std::vector<ssp4sim::graph::Invocable *> &nodes)
    {
        std::size_t total = 0;
        for (auto *n : nodes)
        {
            total += static_cast<MockNode *>(n)->invoke_count;
        }
        return total;
    }

    constexpr uint64_t T0 = 0;
    // The configured timestep (1200 ns, one macro step == T1) so build()
    // returns an executor whose single macro step covers exactly [0, 1200)).
    // 1200 is chosen so the Linear-mode sub-step split (sub_dt = macro / n,
    // integer division) yields exactly n sub-steps for every iteration count
    // used in these tests (2, 3, 4): 1200 / 2 = 600, 1200 / 3 = 400,
    // 1200 / 4 = 300.
    constexpr uint64_t T1 = 1200;

    ssp4sim::ExecutorOptions la2_options(const char *mode, int iterations)
    {
        ssp4sim::ExecutorOptions options;
        options.method = "la2";
        options.la2.mode = mode;
        options.la2.iterations = iterations;
        return options;
    }
} // namespace

TEST_CASE("ExecutorOptions::load reads la2.* keys and ignores legacy loop_aware.*",
          "[la2][config][compat]")
{
    SECTION("legacy loop_aware keys alone fall back to la2 defaults")
    {
        Config::loadFromString(R"json({
            "simulation": {
                "timestep": 1e-6,
                "executor": {
                    "method": "la2",
                    "loop_aware": { "iterations": 4, "mode": "fixed" }
                }
            }
        })json");

        const auto options = ssp4sim::ExecutorOptions::load();
        REQUIRE(options.method == "la2");
        REQUIRE(options.la2.iterations == -1);   // default: SCC node count
        REQUIRE(options.la2.mode == "linear");
        REQUIRE(options.la2.parallel == false);
    }

    SECTION("la2.* keys are honored alongside ignored legacy loop_aware.* keys")
    {
        Config::loadFromString(R"json({
            "simulation": {
                "timestep": 1e-6,
                "executor": {
                    "method": "la2",
                    "la2": { "iterations": 2, "mode": "linear" },
                    "loop_aware": { "iterations": 6, "mode": "fixed" }
                }
            }
        })json");

        const auto options = ssp4sim::ExecutorOptions::load();
        REQUIRE(options.la2.iterations == 2);
        REQUIRE(options.la2.mode == "linear");
    }
}

TEST_CASE("executor_builder accepts legacy 'loop_aware' method name", "[la2][config][compat]")
{
    auto options = la2_options("linear", 2);
    options.method = "loop_aware";

    std::vector<std::shared_ptr<MockNode>> storage;
    make_chain_graph(storage);

    ssp4sim::graph::ExecutorBuilder builder(options, T1);
    auto executor = builder.build(to_owned(storage));

    REQUIRE(executor != nullptr);
    // build() wraps the specialized executor in a MacroExecutor; the legacy
    // name must dispatch to the la2 strategy's SerialSeidel inside, not to a
    // fallback.
    auto *macro = dynamic_cast<ssp4sim::graph::MacroExecutor *>(executor.get());
    REQUIRE(macro != nullptr);
    REQUIRE(macro->nodes.size() == 1);
    REQUIRE(dynamic_cast<ssp4sim::graph::SerialSeidel *>(macro->nodes[0].get()) != nullptr);
}

TEST_CASE("legacy mode aliases map to the new scheduler modes", "[la2][config][compat]")
{
    SECTION("'fixed' behaves like 'linear' (equal sub-steps, count = iterations)")
    {
        std::vector<std::shared_ptr<MockNode>> storage;
        auto nodes = make_loop_graph(storage);

        ssp4sim::graph::ExecutorBuilder builder(la2_options("fixed", 3), T1);
        auto executor = builder.build(to_owned(storage));
        executor->invoke(ssp4sim::graph::StepData(T0, T1));

        // 2 nodes * 3 equal sub-steps = 6 invocations.
        REQUIRE(total_invocations(nodes) == 6);
    }

    SECTION("'geometric' behaves like 'factor' (shrinking sub-steps, threshold cutoff)")
    {
        std::vector<std::shared_ptr<MockNode>> storage;
        auto nodes = make_loop_graph(storage);

        // iterations only drives the linear split; the factor mode sub-steps
        // shrink by `factor` until the remaining time is at or below
        // `threshold` (ns; the config loader converts seconds to ns, so the
        // typed struct is set in ns here).
        auto options = la2_options("geometric", 4);
        options.la2.factor = 0.5;
        options.la2.threshold = 150; // ns -> 3 sub-steps over [0, 1200)

        ssp4sim::graph::ExecutorBuilder builder(options, T1);
        auto executor = builder.build(to_owned(storage));
        executor->invoke(ssp4sim::graph::StepData(T0, T1));

        // 2 nodes * 3 shrinking sub-steps = 6 invocations.
        REQUIRE(total_invocations(nodes) == 6);
    }

    SECTION("'geometric' with factor outside (0,1) is rejected")
    {
        std::vector<std::shared_ptr<MockNode>> storage;
        auto nodes = make_loop_graph(storage);

        auto options = la2_options("geometric", 3);
        options.la2.factor = 1.5;

        ssp4sim::graph::ExecutorBuilder builder(options, T1);
        // GeometricSubstepExecutor rejects a shrink factor outside (0, 1);
        // the legacy equal-sub-step fallback was removed.
        REQUIRE_THROWS_AS(builder.build(to_owned(storage)), std::runtime_error);
    }
}

TEST_CASE("executor_builder throws a clear error for parallel_seidel",
          "[la2][config][compat]")
{
    ssp4sim::ExecutorOptions options;
    options.method = "seidel";
    options.seidel_parallel = true;

    std::vector<std::shared_ptr<MockNode>> storage;
    make_chain_graph(storage);

    ssp4sim::graph::ExecutorBuilder builder(options, T1);
    REQUIRE_THROWS_AS(builder.build(to_owned(storage)), std::runtime_error);

    // Also verify the message is clear and actionable.
    try
    {
        std::vector<std::shared_ptr<MockNode>> single;
        single.push_back(std::make_shared<MockNode>("A"));
        builder.build(to_owned(single));
        FAIL("expected std::runtime_error");
    }
    catch (const std::runtime_error &e)
    {
        REQUIRE(std::string(e.what()).find("ParallelSeidel") != std::string::npos);
        REQUIRE(std::string(e.what()).find("NOT implemented") != std::string::npos);
    }
}

TEST_CASE("unknown executor method still throws with the method name",
          "[la2][config][compat]")
{
    ssp4sim::ExecutorOptions options;
    options.method = "definitely_not_a_method";

    std::vector<std::shared_ptr<MockNode>> storage;
    make_chain_graph(storage);

    ssp4sim::graph::ExecutorBuilder builder(options, T1);
    REQUIRE_THROWS_AS(builder.build(to_owned(storage)), std::runtime_error);

    // The message must include the received method name.
    try
    {
        std::vector<std::shared_ptr<MockNode>> single;
        single.push_back(std::make_shared<MockNode>("A"));
        builder.build(to_owned(single));
        FAIL("expected std::runtime_error");
    }
    catch (const std::runtime_error &e)
    {
        REQUIRE(std::string(e.what()).find("definitely_not_a_method") != std::string::npos);
    }
}