#include "execution/substep/substep_schedule.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <utility>
#include <vector>

namespace
{
    using ssp4sim::graph::substep::build_substep_schedule;
    using ssp4sim::graph::substep::Config;
    using ssp4sim::graph::substep::Mode;
}

// ---------------------------------------------------------------------------
// Description: Linear mode pre-selects equal sub-steps covering the macro step.
// ---------------------------------------------------------------------------
TEST_CASE("Substep schedule linear divides the macro step equally", "[substep_schedule]")
{
    Config cfg;
    cfg.mode = Mode::Linear;
    cfg.steps = 4;

    auto sched = build_substep_schedule(0, 100, cfg);
    REQUIRE(sched.size() == 4);
    CHECK(sched[0] == std::make_pair<uint64_t, uint64_t>(0, 25));
    CHECK(sched[1] == std::make_pair<uint64_t, uint64_t>(25, 50));
    CHECK(sched[2] == std::make_pair<uint64_t, uint64_t>(50, 75));
    CHECK(sched[3] == std::make_pair<uint64_t, uint64_t>(75, 100));
}

// ---------------------------------------------------------------------------
// Description: Linear mode clamps the final sub-step at the macro end when the
//              macro duration isn't divisible by steps, mirroring the legacy
//              "fixed" behaviour.
// ---------------------------------------------------------------------------
TEST_CASE("Substep schedule linear clamps non-divisible macros", "[substep_schedule]")
{
    Config cfg;
    cfg.mode = Mode::Linear;
    cfg.steps = 4;

    auto sched = build_substep_schedule(0, 10, cfg);
    REQUIRE(sched.size() == 5);
    CHECK(sched[0] == std::make_pair<uint64_t, uint64_t>(0, 2));
    CHECK(sched[1] == std::make_pair<uint64_t, uint64_t>(2, 4));
    CHECK(sched[2] == std::make_pair<uint64_t, uint64_t>(4, 6));
    CHECK(sched[3] == std::make_pair<uint64_t, uint64_t>(6, 8));
    CHECK(sched[4] == std::make_pair<uint64_t, uint64_t>(8, 10));
}

// ---------------------------------------------------------------------------
// Description: Factor mode with steps > 0 scales the shrinking sub-steps to
//              land exactly on the macro end (geometric semantics).
// ---------------------------------------------------------------------------
TEST_CASE("Substep schedule factor scaled lands exactly on the macro end", "[substep_schedule]")
{
    Config cfg;
    cfg.mode = Mode::Factor;
    cfg.steps = 3;
    cfg.factor = 0.5;

    auto sched = build_substep_schedule(0, 100, cfg);
    REQUIRE(sched.size() == 3);
    CHECK(sched[0] == std::make_pair<uint64_t, uint64_t>(0, 57));
    CHECK(sched[1] == std::make_pair<uint64_t, uint64_t>(57, 86));
    CHECK(sched[2] == std::make_pair<uint64_t, uint64_t>(86, 100));
}

// ---------------------------------------------------------------------------
// Description: Factor free-shrink (steps == 0) halves the remaining time until
//              the next candidate sub-step is shorter than min_substep, then
//              extends the last sub-step to cover the remaining macro tail, so
//              the whole macro step is simulated (full-coverage contract).
// ---------------------------------------------------------------------------
TEST_CASE("Substep schedule factor free-shrink fully covers the macro interval", "[substep_schedule]")
{
    Config cfg;
    cfg.mode = Mode::Factor;
    cfg.factor = 0.5;
    cfg.min_substep = 10;
    cfg.max_steps = 100;

    auto sched = build_substep_schedule(0, 100, cfg);
    REQUIRE(sched.size() == 3);
    CHECK(sched[0] == std::make_pair<uint64_t, uint64_t>(0, 50));
    CHECK(sched[1] == std::make_pair<uint64_t, uint64_t>(50, 75));
    // Last sub-step is extended to the macro end (was (75, 88)) so the tail
    // 88 -> 100 is simulated and the union of sub-steps equals [0, 100].
    CHECK(sched[2] == std::make_pair<uint64_t, uint64_t>(75, 100));
}

// ---------------------------------------------------------------------------
// Description: Free-shrink also honors min_substep threshold and degenerate
//              inputs produce an empty schedule.
// ---------------------------------------------------------------------------
TEST_CASE("Substep schedule factor free-shrink min threshold and empty inputs", "[substep_schedule]")
{
    Config cfg;
    cfg.mode = Mode::Factor;
    cfg.factor = 0.5;
    cfg.min_substep = 25;
    cfg.max_steps = 100;

    auto sched = build_substep_schedule(0, 100, cfg);
    REQUIRE(sched.size() == 2);
    CHECK(sched[0] == std::make_pair<uint64_t, uint64_t>(0, 50));
    CHECK(sched[1] == std::make_pair<uint64_t, uint64_t>(50, 100));

    // start >= end -> empty.
    CHECK(build_substep_schedule(100, 100, cfg).empty());
}

// ---------------------------------------------------------------------------
// Description: Regression (a) — free-shrink with a large macro and the default
//              min_substep_fraction (0.001) must fully cover the macro interval
//              with no uncovered tail.
// ---------------------------------------------------------------------------
TEST_CASE("Substep schedule factor free-shrink large macro full coverage", "[substep_schedule]")
{
    Config cfg;
    cfg.mode = Mode::Factor;
    cfg.factor = 0.5;
    cfg.min_substep = 1000; // 0.001 of macro = default-ish min_substep_fraction
    cfg.max_steps = 100;

    auto sched = build_substep_schedule(0, 1000000, cfg);
    REQUIRE_FALSE(sched.empty());
    CHECK(sched.front().first == 0);
    CHECK(sched.back().second == 1000000);
    // Union of all sub-steps is contiguous and reaches the macro end (no gap).
    auto prev_end = sched.front().first;
    for (const auto &[s, e] : sched)
    {
        CHECK(s == prev_end);
        prev_end = e;
    }
    CHECK(prev_end == 1000000);
}

// ---------------------------------------------------------------------------
// Description: Regression (b) — fixed scaled factor mode (steps > 0) with a
//              tiny macro: llround collisions collapse inner sub-steps to zero
//              length and they are dropped, but the last sub-step must still
//              reach the macro end so the interval [0, 1] is fully covered.
// ---------------------------------------------------------------------------
TEST_CASE("Substep schedule factor scaled tiny macro fully covers the interval", "[substep_schedule]")
{
    Config cfg;
    cfg.mode = Mode::Factor;
    cfg.steps = 3;
    cfg.factor = 0.5;

    auto sched = build_substep_schedule(0, 1, cfg);
    REQUIRE_FALSE(sched.empty());
    CHECK(sched.front().first == 0);
    CHECK(sched.back().second == 1);
    // Zero-length sub-steps are dropped, but what remains stays contiguous and
    // reaches the macro end (the single real sub-step is (0, 1)).
    auto prev_end = sched.front().first;
    for (const auto &[s, e] : sched)
    {
        CHECK(s == prev_end);
        prev_end = e;
    }
    CHECK(prev_end == 1);
}

// ---------------------------------------------------------------------------
// Description: Regression (c) — free-shrink with a macro so small that the
//              first candidate sub-step rounds to zero must still emit a single
//              sub-step covering the whole macro (previously returned empty).
// ---------------------------------------------------------------------------
TEST_CASE("Substep schedule factor free-shrink tiny macro still covers the interval", "[substep_schedule]")
{
    Config cfg;
    cfg.mode = Mode::Factor;
    cfg.factor = 0.7;
    cfg.min_substep = 0;
    cfg.max_steps = 100;

    auto sched = build_substep_schedule(0, 1, cfg);
    REQUIRE(sched.size() == 1);
    CHECK(sched[0] == std::make_pair<uint64_t, uint64_t>(0, 1));

    // The min_substep threshold fires before the tail-close too: min_substep=1
    // on a macro of 2 with r=0.5 leaves the last sub-step extended to `end`.
    Config cfg2;
    cfg2.mode = Mode::Factor;
    cfg2.factor = 0.5;
    cfg2.min_substep = 1;
    cfg2.max_steps = 100;

    auto sched2 = build_substep_schedule(0, 2, cfg2);
    REQUIRE_FALSE(sched2.empty());
    CHECK(sched2.back().second == 2);
}