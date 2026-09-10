// Config-boundary compatibility tests for the la2 scheduler / executor
// builder (patch task P1: findings A, B-keys, C, E).
//
// Verifies that legacy `loop_aware` configurations keep working without
// modification:
// (a) executor_builder builds a MacroExecutor wrapping a La2Scheduler (or a
    //     La2Scheduler directly for method "loop_aware" when unwrapped);
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
//
// Executors share ownership of their nodes (std::shared_ptr), so the test
// builds shared graphs and hands a shared copy to each executor; a raw-pointer
// snapshot is kept for invocation counting.

#include <catch2/catch_test_macros.hpp>

#include "config.hpp"
#include "executor_builder.hpp"
#include "execution/loop_aware/la2_scheduler.hpp"
#include "execution/macro/macro_executor.hpp"

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

    std::vector<std::shared_ptr<MockNode>> storage;
    auto nodes = make_chain_graph(storage);

    ssp4sim::graph::ExecutorBuilder builder;
    auto executor = builder.build(to_owned(storage));

    REQUIRE(executor != nullptr);
    // build() wraps the specialized executor in a MacroExecutor; the legacy name
    // must dispatch to the La2Scheduler inside, not to a fallback.
    auto *macro = dynamic_cast<ssp4sim::graph::MacroExecutor *>(executor.get());
    REQUIRE(macro != nullptr);
    REQUIRE(macro->nodes.size() == 1);
    REQUIRE(dynamic_cast<ssp4sim::graph::La2Scheduler *>(macro->nodes[0].get()) != nullptr);
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

    std::vector<std::shared_ptr<MockNode>> storage;
    auto nodes = make_loop_graph(storage);

    ssp4sim::graph::La2Scheduler scheduler(to_owned(storage));
    scheduler.invoke(ssp4sim::graph::StepData(T0, T1));

    // "fixed" == equal sub-steps, count = iterations = 4. The 2-node loop is
    // relaxed over 4 sub-steps, each node invoked once per sub-step:
    // 2 nodes * 4 sub-steps = 8 invocations.
    REQUIRE(total_invocations(nodes) == 8);
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

    std::vector<std::shared_ptr<MockNode>> storage;
    auto nodes = make_loop_graph(storage);

    ssp4sim::graph::La2Scheduler scheduler(to_owned(storage));
    scheduler.invoke(ssp4sim::graph::StepData(T0, T1));

    // 2 nodes * 2 sub-steps = 4 invocations (not 12).
    REQUIRE(total_invocations(nodes) == 4);
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

        std::vector<std::shared_ptr<MockNode>> storage;
        auto nodes = make_loop_graph(storage);

        ssp4sim::graph::La2Scheduler scheduler(to_owned(storage));
        scheduler.invoke(ssp4sim::graph::StepData(T0, T1));

        // 2 nodes * 3 equal sub-steps = 6 invocations.
        REQUIRE(total_invocations(nodes) == 6);
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

        std::vector<std::shared_ptr<MockNode>> storage;
        auto nodes = make_loop_graph(storage);

        ssp4sim::graph::La2Scheduler scheduler(to_owned(storage));
        scheduler.invoke(ssp4sim::graph::StepData(T0, T1));

        // "geometric" honors BOTH iterations (sub-step count) and factor:
        // 2 nodes * 4 shrinking sub-steps = 8 invocations.
        REQUIRE(total_invocations(nodes) == 8);
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

        std::vector<std::shared_ptr<MockNode>> storage;
        auto nodes = make_loop_graph(storage);

        ssp4sim::graph::La2Scheduler scheduler(to_owned(storage));
        scheduler.invoke(ssp4sim::graph::StepData(T0, T1));

        // build_substep_schedule treats an out-of-range factor as Linear:
        // 2 nodes * 3 equal sub-steps = 6 invocations.
        REQUIRE(total_invocations(nodes) == 6);
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

    std::vector<std::shared_ptr<MockNode>> storage;
    make_chain_graph(storage);

    ssp4sim::graph::ExecutorBuilder builder;
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
    Config::loadFromString(R"json({
        "simulation": {
            "timestep": 1e-6,
            "executor": {
                "method": "definitely_not_a_method"
            }
        }
    })json");

    std::vector<std::shared_ptr<MockNode>> storage;
    make_chain_graph(storage);

    ssp4sim::graph::ExecutorBuilder builder;
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