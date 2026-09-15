// Variant-registry tests for ExecutorBuilder.
//
// Verifies that every concrete executor registers as a leaf variant and that
// build() resolves the single matching variant from the typed config set
// (`ssp4sim::ExecutorOptions`) — no per-family branching:
//   (a) jacobi: serial vs the three parallel backends (TBB / spin / futures).
//   (b) seidel: serial vs the parallel stub.
//   (c) custom delay family: `custom_delay` / `custom_delay_partial`.
//   (d) legacy `loop_aware` alias resolves to the same la2 variant.
//   (e) `realtime` selects the RealtimeMacroExecutor outer wrap.
//   (f) a config set with no matching variant throws, naming the method
//       (unknown method, and an unknown jacobi parallel backend id).
//
// Behavior is verified through `ExecutorBuilder` over typed `ExecutorOptions`;
// no executor reads the global utils::Config.
//
// Executors share ownership of their nodes (std::shared_ptr), so the test
// builds shared graphs and hands a shared copy to each executor. The shared
// pointer returned by build() must be KEPT alive while the wrapped executor is
// inspected (the test_la2_config_compat.cpp pattern): inspection happens in
// `specialized_of` / the realtime assertions a statement that still holds the
// owning shared_ptr.

#include <catch2/catch_test_macros.hpp>

#include "executor/custom/custom_executors.hpp"
#include "executor/jacobi/jacobi_parallel_fut.hpp"
#include "executor/jacobi/jacobi_parallel_spin.hpp"
#include "executor/jacobi/jacobi_parallel_tbb.hpp"
#include "executor/jacobi/jacobi_serial.hpp"
#include "executor/seidel/seidel_parallel.hpp"
#include "executor/seidel/seidel_serial.hpp"
#include "executor_builder.hpp"
#include "executor/macro/macro_executor.hpp"
#include "executor/macro/realtime_macro_executor.hpp"
#include "shared_config.hpp"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

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

    // Copy the shared storage into a shared Invocable vector for an executor.
    std::vector<std::shared_ptr<ssp4sim::graph::Invocable>> to_owned(
        const std::vector<std::shared_ptr<MockNode>> &storage)
    {
        return std::vector<std::shared_ptr<ssp4sim::graph::Invocable>>(
            storage.begin(), storage.end());
    }

    // A -> B -> C: three single-node SCCs (no loops).
    std::vector<std::shared_ptr<MockNode>> make_chain_graph()
    {
        std::vector<std::shared_ptr<MockNode>> storage;
        storage.push_back(std::make_shared<MockNode>("A"));
        storage.push_back(std::make_shared<MockNode>("B"));
        storage.push_back(std::make_shared<MockNode>("C"));
        auto *a = storage[0].get();
        auto *b = storage[1].get();
        a->add_child(b);
        auto *c = storage[2].get();
        b->add_child(c);
        return storage;
    }

    // The custom delay executors look up named nodes (Sources, LET1..5,
    // C1..4); build the flat 10-node bag they require.
    std::vector<std::shared_ptr<MockNode>> make_delay_graph()
    {
        std::vector<std::shared_ptr<MockNode>> storage;
        for (const auto &name : {"Sources", "LET1", "LET2", "LET3", "LET4",
                                 "LET5", "C1", "C2", "C3", "C4"})
        {
            storage.push_back(std::make_shared<MockNode>(name));
        }
        return storage;
    }

    constexpr uint64_t T1 = 1200;

    // Unwrap the MacroExecutor build() returns and hand back its single
    // specialized child. `executor` must outlive the caller's use of the
    // returned pointer.
    ssp4sim::graph::ExecutorBase *specialized_of(std::shared_ptr<ssp4sim::graph::ExecutorBase> executor)
    {
        auto *macro = dynamic_cast<ssp4sim::graph::MacroExecutor *>(executor.get());
        REQUIRE(macro != nullptr);
        REQUIRE(macro->nodes.size() == 1);
        return dynamic_cast<ssp4sim::graph::ExecutorBase *>(macro->nodes[0].get());
    }
} // namespace

TEST_CASE("jacobi variants resolve to the four concrete executors", "[builder][variant][jacobi]")
{
    ssp4sim::ExecutorOptions options;
    options.method = "jacobi";

    SECTION("jacobi_parallel=false selects JacobiSerial")
    {
        auto storage = make_chain_graph();
        ssp4sim::graph::ExecutorBuilder builder(options, T1);
        auto executor = builder.build(to_owned(storage));
        REQUIRE(dynamic_cast<ssp4sim::graph::JacobiSerial *>(specialized_of(executor)) != nullptr);
    }

    SECTION("method=1 selects JacobiParallelTBB")
    {
        options.jacobi_parallel = true;
        options.jacobi_method = 1;
        auto storage = make_chain_graph();
        ssp4sim::graph::ExecutorBuilder builder(options, T1);
        auto executor = builder.build(to_owned(storage));
        REQUIRE(dynamic_cast<ssp4sim::graph::JacobiParallelTBB *>(specialized_of(executor)) != nullptr);
    }

    SECTION("method=2 selects JacobiParallelSpin")
    {
        options.jacobi_parallel = true;
        options.jacobi_method = 2;
        auto storage = make_chain_graph();
        ssp4sim::graph::ExecutorBuilder builder(options, T1);
        auto executor = builder.build(to_owned(storage));
        REQUIRE(dynamic_cast<ssp4sim::graph::JacobiParallelSpin *>(specialized_of(executor)) != nullptr);
    }

    SECTION("method=3 selects JacobiParallelFutures")
    {
        options.jacobi_parallel = true;
        options.jacobi_method = 3;
        auto storage = make_chain_graph();
        ssp4sim::graph::ExecutorBuilder builder(options, T1);
        auto executor = builder.build(to_owned(storage));
        REQUIRE(dynamic_cast<ssp4sim::graph::JacobiParallelFutures *>(specialized_of(executor)) != nullptr);
    }
}

TEST_CASE("seidel variants resolve to serial vs the parallel stub", "[builder][variant][seidel]")
{
    ssp4sim::ExecutorOptions options;
    options.method = "seidel";

    SECTION("seidel_parallel=false selects SerialSeidel")
    {
        auto storage = make_chain_graph();
        ssp4sim::graph::ExecutorBuilder builder(options, T1);
        auto executor = builder.build(to_owned(storage));
        REQUIRE(dynamic_cast<ssp4sim::graph::SerialSeidel *>(specialized_of(executor)) != nullptr);
    }

    SECTION("seidel_parallel=true selects ParallelSeidel (runtime stub)")
    {
        options.seidel_parallel = true;
        auto storage = make_chain_graph();
        ssp4sim::graph::ExecutorBuilder builder(options, T1);
        auto executor = builder.build(to_owned(storage));
        REQUIRE(dynamic_cast<ssp4sim::graph::ParallelSeidel *>(specialized_of(executor)) != nullptr);
    }
}

TEST_CASE("custom delay variants resolve to DelayExecutor and DelayExecutorPartial",
          "[builder][variant][custom]")
{
    ssp4sim::ExecutorOptions options;

    SECTION("custom_delay selects DelayExecutor")
    {
        options.method = "custom_delay";
        auto storage = make_delay_graph();
        ssp4sim::graph::ExecutorBuilder builder(options, T1);
        auto executor = builder.build(to_owned(storage));
        REQUIRE(dynamic_cast<ssp4sim::graph::DelayExecutor *>(specialized_of(executor)) != nullptr);
    }

    SECTION("custom_delay_partial selects DelayExecutorPartial")
    {
        options.method = "custom_delay_partial";
        auto storage = make_delay_graph();
        ssp4sim::graph::ExecutorBuilder builder(options, T1);
        auto executor = builder.build(to_owned(storage));
        REQUIRE(dynamic_cast<ssp4sim::graph::DelayExecutorPartial *>(specialized_of(executor)) != nullptr);
    }
}

TEST_CASE("executor_builder legacy 'loop_aware' method name resolves to the la2 variant",
          "[builder][variant][la2]")
{
    ssp4sim::ExecutorOptions options;
    options.method = "loop_aware";

    auto storage = make_chain_graph();
    ssp4sim::graph::ExecutorBuilder builder(options, T1);
    auto executor = builder.build(to_owned(storage));
    // Both spellings dispatch to the la2 variant's SerialSeidel stack.
    REQUIRE(dynamic_cast<ssp4sim::graph::SerialSeidel *>(specialized_of(executor)) != nullptr);
}

TEST_CASE("realtime selects the RealtimeMacroExecutor outer wrap", "[builder][variant][realtime]")
{
    ssp4sim::ExecutorOptions options;
    options.method = "jacobi";
    options.realtime = true;

    auto storage = make_chain_graph();
    ssp4sim::graph::ExecutorBuilder builder(options, T1);
    auto executor = builder.build(to_owned(storage));

    REQUIRE(dynamic_cast<ssp4sim::graph::RealtimeMacroExecutor *>(executor.get()) != nullptr);
    REQUIRE(dynamic_cast<ssp4sim::graph::MacroExecutor *>(executor.get()) == nullptr);
}

TEST_CASE("a config set with no matching variant throws, naming the method",
          "[builder][variant][nomatch]")
{
    auto check_message = [](const std::vector<std::shared_ptr<MockNode>> &storage,
                            ssp4sim::graph::ExecutorBuilder &b,
                            std::string expected)
    {
        try
        {
            b.build(to_owned(storage));
            FAIL("expected std::runtime_error");
        }
        catch (const std::runtime_error &e)
        {
            REQUIRE(std::string(e.what()).find(expected) != std::string::npos);
        }
    };

    SECTION("unknown method throws a no-match error naming the method")
    {
        ssp4sim::ExecutorOptions options;
        options.method = "definitely_not_a_method";
        ssp4sim::graph::ExecutorBuilder unknown_builder(options, T1);
        auto storage = make_chain_graph();
        REQUIRE_THROWS_AS(unknown_builder.build(to_owned(storage)), std::runtime_error);
        check_message(storage, unknown_builder, "definitely_not_a_method");
    }

    SECTION("unknown jacobi parallel backend id throws a no-match error")
    {
        ssp4sim::ExecutorOptions options;
        options.method = "jacobi";
        options.jacobi_parallel = true;
        options.jacobi_method = 4;
        ssp4sim::graph::ExecutorBuilder bad_backend(options, T1);
        auto storage = make_chain_graph();
        REQUIRE_THROWS_AS(bad_backend.build(to_owned(storage)), std::runtime_error);
        check_message(storage, bad_backend, "No executor variant matches config");
    }
}