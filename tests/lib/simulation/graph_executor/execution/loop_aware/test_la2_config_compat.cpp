// Config-boundary compatibility tests for the la2 scheduler / executor
// builder (patch task P1: findings A, B-keys, C, E).
//
// Verifies that legacy `loop_aware` configurations keep working without
// modification:
//   (a) executor_builder builds a La2Scheduler for method "loop_aware";
//   (b) legacy `simulation.executor.loop_aware.*` keys are picked up when the
//       `simulation.executor.la2.*` keys are absent;
//   (c) `la2.*` keys take precedence over `loop_aware.*` keys;
//   (d) legacy mode values "fixed" / "geometric" map to the same behavior as
//       "linear" / "factor";
//   (e) requesting "parallel_seidel" throws a clear std::runtime_error at
//       dispatch time.
//
// NOTE: utils::Config is an all-static class with no per-key set()/reset()
// API; tests replace the whole document via Config::loadFromString(...) per
// SECTION (same pattern as tests/lib/utils/test_config.cpp).

#include <catch2/catch_test_macros.hpp>

#include "config.hpp"
#include "execution/executor_builder.hpp"
#include "execution/loop_aware/la2_scheduler.hpp"

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

    // A -> B -> A: one 2-node loop SCC (the only SCC).
    std::vector<ssp4sim::graph::Invocable *> make_loop_graph(
        std::vector<std::unique_ptr<MockNode>> &storage)
    {
        storage.clear();
        storage.push_back(std::make_unique<MockNode>("A"));
        storage.push_back(std::make_unique<MockNode>("B"));
        auto *a = storage[0].get();
        auto *b = storage[1].get();
        a->add_child(b);
        b->add_child(a);
        return {a, b};
    }

    // A -> B -> C: three single-node SCCs (no loops).
    std::vector<ssp4sim::graph::Invocable *> make_chain_graph(
        std::vector<std::unique_ptr<MockNode>> &storage)
    {
        storage.clear();
        storage.push_back(std::make_unique<MockNode>("A"));
        storage.push_back(std::make_unique<MockNode>("B"));
        storage.push_back(std::make_unique<MockNode>("C"));
        auto *a = storage[0].get();
        auto *b = storage[1].get();
        auto *c = storage[2].get();
        a->add_child(b);
        b->add_child(c);
        return {a, b, c};
    }

    // Total invocations across all mock nodes.
    std::size_t total_invocations(const std::vector<std::unique_ptr<MockNode>> &storage)
    {
        std::size_t total = 0;
        for (const auto &n : storage)
        {
            total += n->invoke_count;
        }
        return total;
    }

    constexpr uint64_t T0 = 0;
    // Macro step [0, 1200) ns. Chosen so that the Linear-mode sub-step split
    // (sub_dt = macro / n, integer division) yields exactly n sub-steps for
    // every iteration count used in these tests (2, 3, 4):
    //   1200 / 2 = 600, 1200 / 3 = 400, 1200 / 4 = 300.
    constexpr uint64_t T1 = 1200;

} // namespace

TEST_CASE("executor_builder accepts legacy 'loop_aware' method name", "[la2][config][compat]")
{
    Config::loadFromString(R"json({
        "simulation": {
            "timestep": 1e-6,
            "executor": {
                "method": "loop_aware"
            }
        }
    })json");

    std::vector<std::unique_ptr<MockNode>> storage;
    auto nodes = make_chain_graph(storage);

    ssp4sim::graph::ExecutorBuilder builder;
    auto executor = builder.build(nodes);

    REQUIRE(executor != nullptr);
    // The legacy name must dispatch to the La2Scheduler, not to a fallback.
    REQUIRE(dynamic_cast<ssp4sim::graph::La2Scheduler *>(executor.get()) != nullptr);
}

TEST_CASE("legacy loop_aware config keys are honored when la2.* keys are absent",
          "[la2][config][compat]")
{
    // Only the legacy namespace is present.
    Config::loadFromString(R"json({
        "simulation": {
            "timestep": 1e-6,
            "executor": {
                "method": "la2",
                "loop_aware": {
                    "iterations": 4,
                    "mode": "fixed"
                }
            }
        }
    })json");

    std::vector<std::unique_ptr<MockNode>> storage;
    auto nodes = make_loop_graph(storage);

    ssp4sim::graph::La2Scheduler scheduler(nodes);
    scheduler.invoke(ssp4sim::graph::StepData(T0, T1, T1 - T0));

    // "fixed" == equal sub-steps, count = iterations = 4. The 2-node loop is
    // relaxed over 4 sub-steps, each node invoked once per sub-step:
    // 2 nodes * 4 sub-steps = 8 invocations.
    REQUIRE(total_invocations(storage) == 8);
}

TEST_CASE("la2.* config keys take precedence over legacy loop_aware.* keys",
          "[la2][config][compat]")
{
    // Both namespaces present: la2.iterations=2 must win over
    // loop_aware.iterations=6.
    Config::loadFromString(R"json({
        "simulation": {
            "timestep": 1e-6,
            "executor": {
                "method": "la2",
                "la2": {
                    "iterations": 2,
                    "mode": "linear"
                },
                "loop_aware": {
                    "iterations": 6,
                    "mode": "fixed"
                }
            }
        }
    })json");

    std::vector<std::unique_ptr<MockNode>> storage;
    auto nodes = make_loop_graph(storage);

    ssp4sim::graph::La2Scheduler scheduler(nodes);
    scheduler.invoke(ssp4sim::graph::StepData(T0, T1, T1 - T0));

    // 2 nodes * 2 sub-steps = 4 invocations (not 12).
    REQUIRE(total_invocations(storage) == 4);
}

TEST_CASE("legacy mode aliases map to the new scheduler modes", "[la2][config][compat]")
{
    SECTION("'fixed' behaves like 'linear' (equal sub-steps, count = iterations)")
    {
        Config::loadFromString(R"json({
            "simulation": {
                "timestep": 1e-6,
                "executor": {
                    "method": "la2",
                    "loop_aware": {
                        "iterations": 3,
                        "mode": "fixed"
                    }
                }
            }
        })json");

        std::vector<std::unique_ptr<MockNode>> storage;
        auto nodes = make_loop_graph(storage);

        ssp4sim::graph::La2Scheduler scheduler(nodes);
        scheduler.invoke(ssp4sim::graph::StepData(T0, T1, T1 - T0));

        // 2 nodes * 3 equal sub-steps = 6 invocations.
        REQUIRE(total_invocations(storage) == 6);
    }

    SECTION("'geometric' behaves like 'factor' (shrinking sub-steps, count = iterations)")
    {
        Config::loadFromString(R"json({
            "simulation": {
                "timestep": 1e-6,
                "executor": {
                    "method": "la2",
                    "loop_aware": {
                        "iterations": 4,
                        "mode": "geometric",
                        "factor": 0.5
                    }
                }
            }
        })json");

        std::vector<std::unique_ptr<MockNode>> storage;
        auto nodes = make_loop_graph(storage);

        ssp4sim::graph::La2Scheduler scheduler(nodes);
        scheduler.invoke(ssp4sim::graph::StepData(T0, T1, T1 - T0));

        // "geometric" honors BOTH iterations (sub-step count) and factor:
        // 2 nodes * 4 shrinking sub-steps = 8 invocations.
        REQUIRE(total_invocations(storage) == 8);
    }

    SECTION("'geometric' with factor outside (0,1) falls back to equal sub-steps")
    {
        Config::loadFromString(R"json({
            "simulation": {
                "timestep": 1e-6,
                "executor": {
                    "method": "la2",
                    "loop_aware": {
                        "iterations": 3,
                        "mode": "geometric",
                        "factor": 1.5
                    }
                }
            }
        })json");

        std::vector<std::unique_ptr<MockNode>> storage;
        auto nodes = make_loop_graph(storage);

        ssp4sim::graph::La2Scheduler scheduler(nodes);
        scheduler.invoke(ssp4sim::graph::StepData(T0, T1, T1 - T0));

        // build_substep_schedule treats an out-of-range factor as Linear:
        // 2 nodes * 3 equal sub-steps = 6 invocations.
        REQUIRE(total_invocations(storage) == 6);
    }
}

TEST_CASE("executor_builder throws a clear error for parallel_seidel",
          "[la2][config][compat]")
{
    Config::loadFromString(R"json({
        "simulation": {
            "timestep": 1e-6,
            "executor": {
                "method": "parallel_seidel"
            }
        }
    })json");

    std::vector<std::unique_ptr<MockNode>> storage;
    auto nodes = make_chain_graph(storage);

    ssp4sim::graph::ExecutorBuilder builder;
    REQUIRE_THROWS_AS(builder.build(nodes), std::runtime_error);

    // Also verify the message is clear and actionable.
    try
    {
        builder.build(nodes);
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
    Config::loadFromString(R"json({
        "simulation": {
            "timestep": 1e-6,
            "executor": {
                "method": "definitely_not_a_method"
            }
        }
    })json");

    std::vector<std::unique_ptr<MockNode>> storage;
    auto nodes = make_chain_graph(storage);

    ssp4sim::graph::ExecutorBuilder builder;
    REQUIRE_THROWS_AS(builder.build(nodes), std::runtime_error);

    // The message must include the received method name.
    try
    {
        builder.build(nodes);
        FAIL("expected std::runtime_error");
    }
    catch (const std::runtime_error &e)
    {
        REQUIRE(std::string(e.what()).find("definitely_not_a_method") != std::string::npos);
    }
}