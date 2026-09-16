// Unit tests for the self-contained substep relaxation executors
// (LinearSubstepExecutor / GeometricSubstepExecutor): LinearSubstepExecutor
// builds its schedule inline in invoke() (equal sub-steps, remainder spread
// over the first sub-steps); GeometricSubstepExecutor owns its build_schedule
// static. Both sweep their group in parallel per sub-step.

#include <catch2/catch_test_macros.hpp>

#include "executor/substep/geometric_substep_executor.hpp"
#include "executor/substep/linear_substep_executor.hpp"

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace
{
    using Step = std::pair<std::uint64_t, std::uint64_t>;

    // Invocable that records every (start, end) step it was invoked with.
    class RecordingNode final : public ssp4sim::graph::Invocable
    {
    public:
        RecordingNode(std::string name, std::vector<Step> *steps)
            : steps(steps)
        {
            this->name = std::move(name);
        }

        std::uint64_t invoke(ssp4sim::graph::StepData step) override
        {
            steps->emplace_back(step.start_time, step.end_time);
            return step.end_time;
        }

        std::vector<Step> *steps = nullptr;
    };

    struct NodeBundle
    {
        std::vector<std::shared_ptr<ssp4sim::graph::Invocable>> owned;
        std::vector<Step> recorded_a;
        std::vector<Step> recorded_b;
    };

    NodeBundle make_two_nodes()
    {
        NodeBundle b;
        b.recorded_a.reserve(16);
        b.recorded_b.reserve(16);
        auto a = std::make_shared<RecordingNode>("a", &b.recorded_a);
        auto c = std::make_shared<RecordingNode>("b", &b.recorded_b);
        b.owned.push_back(std::move(a));
        b.owned.push_back(std::move(c));
        return b;
    }

    // The union of the recorded sub-steps must be contiguous and exactly cover
    // [start, end].
    void check_full_coverage(std::uint64_t start, std::uint64_t end,
                             const std::vector<Step> &steps)
    {
        REQUIRE_FALSE(steps.empty());
        auto prev_end = steps.front().first;
        REQUIRE(prev_end == start);
        for (const auto &[s, e] : steps)
        {
            CHECK(s == prev_end);
            prev_end = e;
        }
        CHECK(prev_end == end);
    }

    constexpr std::uint64_t T0 = 0;
    constexpr std::uint64_t T1 = 1200; // evenly divisible by 2,3,4 sub-steps
} // namespace

// ---------------------------------------------------------------------------
// Linear schedule
// ---------------------------------------------------------------------------

TEST_CASE("LinearSubstepExecutor divides the macro step equally",
          "[substep_executor][linear]")
{
    auto b = make_two_nodes();
    ssp4sim::graph::LinearSubstepExecutor ex(b.owned, 4);
    ex.invoke(ssp4sim::graph::StepData(0, 100));

    const std::vector<Step> expected{{0, 25}, {25, 50}, {50, 75}, {75, 100}};
    REQUIRE(b.recorded_a == expected);
    REQUIRE(b.recorded_b == expected);
}

TEST_CASE("LinearSubstepExecutor spreads the remainder over the first sub-steps",
          "[substep_executor][linear]")
{
    // 10 is not divisible by 4: each sub-step gets base 2 and the 2-unit
    // remainder is spread over the first two sub-steps, so no tiny tail is
    // left at the macro end.
    auto b = make_two_nodes();
    ssp4sim::graph::LinearSubstepExecutor ex(b.owned, 4);
    ex.invoke(ssp4sim::graph::StepData(0, 10));

    const std::vector<Step> expected{{0, 3}, {3, 6}, {6, 8}, {8, 10}};
    REQUIRE(b.recorded_a == expected);
    REQUIRE(b.recorded_b == expected);
}

TEST_CASE("LinearSubstepExecutor requires iterations >= 1",
          "[substep_executor][linear]")
{
    REQUIRE_THROWS_AS(ssp4sim::graph::LinearSubstepExecutor(
                          std::vector<std::shared_ptr<ssp4sim::graph::Invocable>>{}, 0),
                      std::runtime_error);
}

TEST_CASE("LinearSubstepExecutor sweeps the group once per equal sub-step",
          "[substep_executor][linear]")
{
    auto b = make_two_nodes();
    ssp4sim::graph::LinearSubstepExecutor ex(b.owned, 4);
    ex.invoke(ssp4sim::graph::StepData(T0, T1));

    const std::vector<Step> expected{{0, 300}, {300, 600}, {600, 900}, {900, 1200}};
    REQUIRE(b.recorded_a == expected);
    REQUIRE(b.recorded_b == expected);
}

TEST_CASE("LinearSubstepExecutor with an empty macro step emits zero-length sub-steps",
          "[substep_executor][linear]")
{
    // An empty macro step still sweeps `iterations` zero-length sub-steps
    // (the remainder-distribution loop has no start >= end early-out).
    auto b = make_two_nodes();
    ssp4sim::graph::LinearSubstepExecutor ex(b.owned, 4);
    ex.invoke(ssp4sim::graph::StepData(T1, T1));

    const std::vector<Step> expected(4, Step(T1, T1));
    REQUIRE(b.recorded_a == expected);
    REQUIRE(b.recorded_b == expected);
}

// ---------------------------------------------------------------------------
// Geometric schedule
// ---------------------------------------------------------------------------

TEST_CASE("GeometricSubstepExecutor.build_schedule free-shrink covers the macro",
          "[substep_executor][geometric]")
{
    auto sched = ssp4sim::graph::GeometricSubstepExecutor::build_schedule(
        0, 100, 0.5, 10);
    REQUIRE(sched.size() == 4);
    CHECK(sched[0] == Step(0, 50));
    CHECK(sched[1] == Step(50, 75));
    CHECK(sched[2] == Step(75, 88));
    // Once the remaining time is at or below the threshold (12 -> 6), the
    // rest of the macro step is taken whole, so the union of sub-steps
    // exactly equals [0, 100].
    CHECK(sched[3] == Step(88, 100));
}

TEST_CASE("GeometricSubstepExecutor.build_schedule takes the remaining step at the threshold",
          "[substep_executor][geometric]")
{
    auto sched = ssp4sim::graph::GeometricSubstepExecutor::build_schedule(
        0, 100, 0.5, 25);
    REQUIRE(sched.size() == 2);
    CHECK(sched[0] == Step(0, 50));
    CHECK(sched[1] == Step(50, 100));

    // A threshold >= the macro duration collapses to one step.
    CHECK(ssp4sim::graph::GeometricSubstepExecutor::build_schedule(
              0, 100, 0.5, 100).size() == 1);

    // start >= end -> empty.
    CHECK(ssp4sim::graph::GeometricSubstepExecutor::build_schedule(
              100, 100, 0.5, 25).empty());
}

TEST_CASE("GeometricSubstepExecutor.build_schedule large macro full coverage",
          "[substep_executor][geometric]")
{
    auto sched = ssp4sim::graph::GeometricSubstepExecutor::build_schedule(
        0, 1000000, 0.5, 1000);
    check_full_coverage(0, 1000000, sched);
}

TEST_CASE("GeometricSubstepExecutor.build_schedule free-shrink tiny macro",
          "[substep_executor][geometric]")
{
    // A macro so small that the first candidate sub-step rounds to zero must
    // still emit a single sub-step covering the whole macro.
    auto sched = ssp4sim::graph::GeometricSubstepExecutor::build_schedule(
        0, 1, 0.7, 0);
    REQUIRE(sched.size() == 1);
    CHECK(sched[0] == Step(0, 1));

    // The threshold fires before the tail-close too: threshold=1 on a macro
    // of 2 with r=0.5 collapses to a single step covering the whole macro.
    auto sched2 = ssp4sim::graph::GeometricSubstepExecutor::build_schedule(
        0, 2, 0.5, 1);
    REQUIRE(sched2.size() == 1);
    CHECK(sched2[0] == Step(0, 2));
}

TEST_CASE("GeometricSubstepExecutor rejects an out-of-range factor",
          "[substep_executor][geometric]")
{
    // An invalid shrink factor is rejected, not silently degraded to equal
    // sub-steps (the legacy fallback was removed when the constructor started
    // validating factor).
    REQUIRE_THROWS_AS(ssp4sim::graph::GeometricSubstepExecutor::build_schedule(
                          0, 100, 1.5, 0),
                      std::runtime_error);
    REQUIRE_THROWS_AS(ssp4sim::graph::GeometricSubstepExecutor(
                          std::vector<std::shared_ptr<ssp4sim::graph::Invocable>>{}, 1.5),
                      std::runtime_error);
}

TEST_CASE("GeometricSubstepExecutor sweeps the group per shrinking sub-step",
          "[substep_executor][geometric]")
{
    auto b = make_two_nodes();
    auto steps = ssp4sim::graph::GeometricSubstepExecutor::build_schedule(
        T0, T1, 0.5, 0);
    ssp4sim::graph::GeometricSubstepExecutor ex(b.owned, 0.5, 0);
    ex.invoke(ssp4sim::graph::StepData(T0, T1));

    REQUIRE_FALSE(steps.empty());
    REQUIRE(steps.back().second == T1);
    REQUIRE(b.recorded_a == steps);
    REQUIRE(b.recorded_b == steps);
}