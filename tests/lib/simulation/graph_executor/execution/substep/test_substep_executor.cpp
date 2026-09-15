// Unit tests for the self-contained substep relaxation executors
// (LinearSubstepExecutor / GeometricSubstepExecutor): each owns the schedule
// construction logic (build_schedule) and sweeps its group in parallel per
// sub-step. Covers the schedule edge cases previously tested against the
// monolithic substep::build_substep_schedule.

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

TEST_CASE("LinearSubstepExecutor.build_schedule divides the macro step equally",
          "[substep_executor][linear]")
{
    auto sched = ssp4sim::graph::LinearSubstepExecutor::build_schedule(0, 100, 4);
    REQUIRE(sched.size() == 4);
    CHECK(sched[0] == Step(0, 25));
    CHECK(sched[1] == Step(25, 50));
    CHECK(sched[2] == Step(50, 75));
    CHECK(sched[3] == Step(75, 100));
}

TEST_CASE("LinearSubstepExecutor.build_schedule clamps non-divisible macros",
          "[substep_executor][linear]")
{
    auto sched = ssp4sim::graph::LinearSubstepExecutor::build_schedule(0, 10, 4);
    REQUIRE(sched.size() == 5);
    CHECK(sched[0] == Step(0, 2));
    CHECK(sched[1] == Step(2, 4));
    CHECK(sched[2] == Step(4, 6));
    CHECK(sched[3] == Step(6, 8));
    CHECK(sched[4] == Step(8, 10));
}

TEST_CASE("LinearSubstepExecutor.build_schedule steps 0 is a single full step",
          "[substep_executor][linear]")
{
    auto sched = ssp4sim::graph::LinearSubstepExecutor::build_schedule(0, 100, 0);
    REQUIRE(sched == std::vector<Step>{Step(0, 100)});
    CHECK(ssp4sim::graph::LinearSubstepExecutor::build_schedule(100, 100, 4).empty());
}

TEST_CASE("LinearSubstepExecutor sweeps the group once per equal sub-step",
          "[substep_executor][linear]")
{
    auto b = make_two_nodes();
    ssp4sim::graph::LinearSubstepExecutor ex(b.owned, 4);
    ex.invoke(ssp4sim::graph::StepData(T0, T1));

    auto expected = ssp4sim::graph::LinearSubstepExecutor::build_schedule(T0, T1, 4);
    REQUIRE(expected.size() == 4);
    REQUIRE(b.recorded_a == expected);
    REQUIRE(b.recorded_b == expected);
}

TEST_CASE("LinearSubstepExecutor with an empty macro step invokes nothing",
          "[substep_executor][linear]")
{
    auto b = make_two_nodes();
    ssp4sim::graph::LinearSubstepExecutor ex(b.owned, 4);
    ex.invoke(ssp4sim::graph::StepData(T1, T1));

    REQUIRE(b.recorded_a.empty());
    REQUIRE(b.recorded_b.empty());
}

// ---------------------------------------------------------------------------
// Geometric schedule
// ---------------------------------------------------------------------------

TEST_CASE("GeometricSubstepExecutor.build_schedule scaled lands on the macro end",
          "[substep_executor][geometric]")
{
    auto sched = ssp4sim::graph::GeometricSubstepExecutor::build_schedule(
        0, 100, 0.5, 100, 0, 3);
    REQUIRE(sched.size() == 3);
    CHECK(sched[0] == Step(0, 57));
    CHECK(sched[1] == Step(57, 86));
    CHECK(sched[2] == Step(86, 100));
}

TEST_CASE("GeometricSubstepExecutor.build_schedule free-shrink covers the macro",
          "[substep_executor][geometric]")
{
    auto sched = ssp4sim::graph::GeometricSubstepExecutor::build_schedule(
        0, 100, 0.5, 100, 10, 0);
    REQUIRE(sched.size() == 3);
    CHECK(sched[0] == Step(0, 50));
    CHECK(sched[1] == Step(50, 75));
    // Last sub-step is extended to the macro end (was (75, 88)) so the tail
    // 88 -> 100 is simulated and the union of sub-steps equals [0, 100].
    CHECK(sched[2] == Step(75, 100));
}

TEST_CASE("GeometricSubstepExecutor.build_schedule free-shrink min threshold",
          "[substep_executor][geometric]")
{
    auto sched = ssp4sim::graph::GeometricSubstepExecutor::build_schedule(
        0, 100, 0.5, 100, 25, 0);
    REQUIRE(sched.size() == 2);
    CHECK(sched[0] == Step(0, 50));
    CHECK(sched[1] == Step(50, 100));

    // start >= end -> empty.
    CHECK(ssp4sim::graph::GeometricSubstepExecutor::build_schedule(
              100, 100, 0.5, 100, 25, 0).empty());
}

TEST_CASE("GeometricSubstepExecutor.build_schedule large macro full coverage",
          "[substep_executor][geometric]")
{
    auto sched = ssp4sim::graph::GeometricSubstepExecutor::build_schedule(
        0, 1000000, 0.5, 100, 1000, 0);
    check_full_coverage(0, 1000000, sched);
}

TEST_CASE("GeometricSubstepExecutor.build_schedule scaled tiny macro coverage",
          "[substep_executor][geometric]")
{
    // llround collisions collapse inner sub-steps to zero length and they are
    // dropped, but the last sub-step must still reach the macro end.
    auto sched = ssp4sim::graph::GeometricSubstepExecutor::build_schedule(
        0, 1, 0.5, 100, 0, 3);
    check_full_coverage(0, 1, sched);
}

TEST_CASE("GeometricSubstepExecutor.build_schedule free-shrink tiny macro",
          "[substep_executor][geometric]")
{
    // A macro so small that the first candidate sub-step rounds to zero must
    // still emit a single sub-step covering the whole macro.
    auto sched = ssp4sim::graph::GeometricSubstepExecutor::build_schedule(
        0, 1, 0.7, 100, 0, 0);
    REQUIRE(sched.size() == 1);
    CHECK(sched[0] == Step(0, 1));

    // The min_substep threshold fires before the tail-close too: min_substep=1
    // on a macro of 2 with r=0.5 leaves the last sub-step extended to `end`.
    auto sched2 = ssp4sim::graph::GeometricSubstepExecutor::build_schedule(
        0, 2, 0.5, 100, 1, 0);
    REQUIRE_FALSE(sched2.empty());
    CHECK(sched2.back().second == 2);
}

TEST_CASE("GeometricSubstepExecutor.build_schedule out-of-range factor falls back",
          "[substep_executor][geometric]")
{
    // An invalid shrink factor is treated as equal sub-steps
    // (legacy "fixed"-style behavior).
    auto sched = ssp4sim::graph::GeometricSubstepExecutor::build_schedule(
        0, 100, 1.5, 100, 0, 4);
    REQUIRE(sched.size() == 4);
    CHECK(sched[0] == Step(0, 25));
    CHECK(sched[3] == Step(75, 100));
}

TEST_CASE("GeometricSubstepExecutor sweeps the group per shrinking sub-step",
          "[substep_executor][geometric]")
{
    auto b = make_two_nodes();
    auto steps = ssp4sim::graph::GeometricSubstepExecutor::build_schedule(
        T0, T1, 0.5, 256, 0, 4);
    ssp4sim::graph::GeometricSubstepExecutor ex(b.owned, 0.5, 256, 0.001, 4);
    ex.invoke(ssp4sim::graph::StepData(T0, T1));

    REQUIRE_FALSE(steps.empty());
    REQUIRE(steps.back().second == T1);
    REQUIRE(b.recorded_a == steps);
    REQUIRE(b.recorded_b == steps);
}