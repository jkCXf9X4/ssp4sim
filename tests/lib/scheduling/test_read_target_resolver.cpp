#include "scheduling/read_target_resolver.hpp"
#include "scheduling/read_target_core.hpp"
#include "scheduling/read_resolver_latest_executed.hpp"
#include "scheduling/read_resolver_macro_step_start_time.hpp"

#include "pre/3_simulation/elements/model_connection.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <memory>
#include <vector>

using ssp4sim::scheduling::ReadTargetResolver;
using ssp4sim::scheduling::ResolverConfig;
using ssp4sim::scheduling::ResolvedRead;
using ssp4sim::scheduling::detail::Edge;
using ssp4sim::scheduling::detail::ModelStatus;
using ssp4sim::scheduling::detail::latest_executed_resolver;
using ssp4sim::scheduling::detail::macro_step_start_time_resolver;
using ssp4sim::graph::ConnectionInfo;
using ssp4sim::scheduling::detail::AccessMode;
using ssp4sim::signal::SignalStorage;
using ssp4sim::types::DataType;

namespace
{
    // ModelStatus holds atomics (non-copyable), so it is configured in place.
    void set_committed(ModelStatus &st, std::uint64_t time, std::size_t area,
                       std::uint64_t count = 1)
    {
        st.committed_count.store(count, std::memory_order::release);
        st.committed_time.store(time, std::memory_order::release);
        st.latest_area.store(area, std::memory_order::release);
        st.generation.store(count, std::memory_order::release);
    }

    ResolverConfig default_config()
    {
        return ResolverConfig{};
    }
}

// ---------------------------------------------------------------------------
// Description: Latest mode resolves to the latest committed area index directly —
//              no time, no scan, no input_time needed
// Rationale:   "Latest" means newest committed index (zero-order hold); the index is
//              the deterministic anchor, never the live head (M1a/D2/D7)
// ---------------------------------------------------------------------------
TEST_CASE("LatestExecutedResolver returns the latest committed index", "[ReadTargetResolver]")
{
    Edge e; // mode defaults to Latest
    ModelStatus st;
    set_committed(st, 500, 3);

    ResolvedRead r = latest_executed_resolver()->resolve(e, st, default_config(), 900, 1100);

    REQUIRE(r.valid);
    REQUIRE(r.is_area);
    REQUIRE(r.area == 3); // latest_area, regardless of requested step span
    REQUIRE(r.generation == 1);
}

TEST_CASE("LatestExecutedResolver is invalid before first commit", "[ReadTargetResolver]")
{
    ModelStatus st; // committed_count == 0
    Edge e; // Latest

    ResolvedRead r = latest_executed_resolver()->resolve(e, st, default_config(), 900, 1100);
    REQUIRE_FALSE(r.valid);
}

// ---------------------------------------------------------------------------
// Description: Latest ignores delay/time_offset (index-domain address, not a time search)
// Rationale:   A delay/lookback on the latest index is future work (D8/B); the initial
//              Latest is simply "the newest committed area"
// ---------------------------------------------------------------------------
TEST_CASE("LatestExecutedResolver ignores delay and time_offset", "[ReadTargetResolver]")
{
    ModelStatus st;
    set_committed(st, 10'000, 5);

    Edge e; e.mode = AccessMode::Latest; e.delay = 2; e.time_offset = 5;
    ResolvedRead r = latest_executed_resolver()->resolve(e, st, default_config(), 900, 1100);
    REQUIRE(r.valid);
    REQUIRE(r.is_area);
    REQUIRE(r.area == 5);
}

// ---------------------------------------------------------------------------
// Description: The "latest executed" choice overrides time sampling for wired edges:
//              even a StartTime-pinned edge resolves to the newest committed area.
// Rationale:   This is the difference the resolver choice expresses — identical graph
//              facts, different read policy.
// ---------------------------------------------------------------------------
TEST_CASE("LatestExecutedResolver overrides time sampling for any edge", "[ReadTargetResolver]")
{
    ModelStatus st;
    set_committed(st, 10'000, 5);

    Edge e; e.mode = AccessMode::StartTime; e.delay = 100; e.time_offset = 7;
    ResolvedRead r = latest_executed_resolver()->resolve(e, st, default_config(), 900, 1100);
    REQUIRE(r.valid);
    REQUIRE(r.is_area);
    REQUIRE(r.area == 5); // latest_area, not 900 - 100 + 7
}

// ---------------------------------------------------------------------------
// Description: StartTime / EndTime select the step handles
// Rationale:   Feedback edges sample start-of-step; delayed edges sample end-of-step
// ---------------------------------------------------------------------------
TEST_CASE("MacroStepStartTimeResolver selects step handles", "[ReadTargetResolver]")
{
    ModelStatus st;
    set_committed(st, 10'000, 1);

    Edge start; start.mode = AccessMode::StartTime;
    ResolvedRead r1 = macro_step_start_time_resolver()->resolve(start, st, default_config(), 100, 200);
    REQUIRE(r1.valid);
    REQUIRE_FALSE(r1.is_area);
    REQUIRE(r1.time == 100);

    Edge end; end.mode = AccessMode::EndTime;
    ResolvedRead r2 = macro_step_start_time_resolver()->resolve(end, st, default_config(), 100, 200);
    REQUIRE(r2.valid);
    REQUIRE_FALSE(r2.is_area);
    REQUIRE(r2.time == 200);
}

// ---------------------------------------------------------------------------
// Description: delay and time_offset shift the (time-domain) StartTime/EndTime reference
// Rationale:   Transport/delay edges and the (currently latent) time_offset knob
// ---------------------------------------------------------------------------
TEST_CASE("MacroStepStartTimeResolver shifts by delay and time_offset", "[ReadTargetResolver]")
{
    ModelStatus st;
    set_committed(st, 10'000, 1);

    Edge delayed; delayed.mode = AccessMode::StartTime; delayed.delay = 3;
    ResolvedRead r = macro_step_start_time_resolver()->resolve(delayed, st, default_config(), 100, 200);
    REQUIRE(r.time == 97); // 100 − 3

    Edge offset; offset.mode = AccessMode::EndTime; offset.time_offset = -10;
    ResolvedRead r2 = macro_step_start_time_resolver()->resolve(offset, st, default_config(), 100, 200);
    REQUIRE(r2.time == 190); // 200 − 10

    Edge both; both.mode = AccessMode::StartTime; both.delay = 2; both.time_offset = 5;
    ResolvedRead r3 = macro_step_start_time_resolver()->resolve(both, st, default_config(), 1000, 1100);
    REQUIRE(r3.time == 1003); // 1000 + 5 − 2
}

// ---------------------------------------------------------------------------
// Description: negative reference is floored at zero (no unsigned underflow, D8)
// Rationale:   Int64 → uint64 cast must never wrap; D2-class first-commit underflow
// ---------------------------------------------------------------------------
TEST_CASE("MacroStepStartTimeResolver floors negative reference at zero", "[ReadTargetResolver]")
{
    ModelStatus st;
    set_committed(st, 10'000, 1);

    Edge e; e.mode = AccessMode::StartTime; e.delay = 500;
    ResolvedRead r = macro_step_start_time_resolver()->resolve(e, st, default_config(), 100, 200);
    REQUIRE(r.valid);
    REQUIRE(r.time == 0);
}

// ---------------------------------------------------------------------------
// Description: clamping is only reachable on time modes; can also be disabled
// Rationale:   Clamp (M1a) is the determinism contract for time-domain sampling
// ---------------------------------------------------------------------------
TEST_CASE("MacroStepStartTimeResolver clamps to the committed frontier", "[ReadTargetResolver]")
{
    ModelStatus st;
    set_committed(st, 500, 1);

    Edge e; e.mode = AccessMode::StartTime;
    ResolvedRead r = macro_step_start_time_resolver()->resolve(e, st, default_config(), 1000, 1100);
    REQUIRE(r.valid);
    REQUIRE(r.time == 500); // clamped to committed frontier

    ResolverConfig cfg; cfg.clamp_stale_shortfall = false;
    ResolvedRead r2 = macro_step_start_time_resolver()->resolve(e, st, cfg, 1000, 1100);
    REQUIRE(r2.valid);
    REQUIRE(r2.time == 1000); // raw, no clamp
}

// ---------------------------------------------------------------------------
// Description: Index mode returns the fixed physical slot as a direct area
// Rationale:   Fixed-slot addressing bypasses time lookup entirely
// ---------------------------------------------------------------------------
TEST_CASE("resolvers keep Index mode at the fixed slot", "[ReadTargetResolver]")
{
    ModelStatus st;
    set_committed(st, 500, 3);

    Edge e; e.mode = AccessMode::Index; e.fixed_index = 7;

    ResolvedRead r = latest_executed_resolver()->resolve(e, st, default_config(), 900, 1100);
    REQUIRE(r.valid);
    REQUIRE(r.is_area);
    REQUIRE(r.area == 7);

    ResolvedRead r2 = macro_step_start_time_resolver()->resolve(e, st, default_config(), 900, 1100);
    REQUIRE(r2.valid);
    REQUIRE(r2.is_area);
    REQUIRE(r2.area == 7);
}

// ---------------------------------------------------------------------------
// Description: unlinked reads are stale-only, direct to the committed area (UC-14)
// Rationale:   No graph edge ⇒ no happens-before ⇒ never the live head
// ---------------------------------------------------------------------------
TEST_CASE("MacroStepStartTimeResolver resolves unlinked reads to committed area only", "[ReadTargetResolver]")
{
    ModelStatus st;
    set_committed(st, 500, 3);

    Edge e; e.mode = AccessMode::StartTime; e.unlinked = true; e.delay = 100;
    ResolvedRead r = macro_step_start_time_resolver()->resolve(e, st, default_config(), 900, 1100);
    REQUIRE(r.valid);
    REQUIRE(r.is_area);
    REQUIRE(r.area == 3); // latest_area, regardless of requested time/delay
    REQUIRE(r.generation == 1);
}

TEST_CASE("MacroStepStartTimeResolver unlinked read invalid before first commit", "[ReadTargetResolver]")
{
    ModelStatus st;
    Edge e; e.mode = AccessMode::Latest; e.unlinked = true;
    ResolvedRead r = macro_step_start_time_resolver()->resolve(e, st, default_config(), 900, 1100);
    REQUIRE_FALSE(r.valid);
}

// ---------------------------------------------------------------------------
// Description: both resolvers advance the same committed frontier on mark_committed
// Rationale:   The shell forwards mark_committed to the injected resolver; either
//              concrete strategy must publish the frontier identically (D17).
// ---------------------------------------------------------------------------
TEST_CASE("resolvers advance the frontier on mark_committed", "[ReadTargetResolver]")
{
    for (const auto resolver : {macro_step_start_time_resolver(), latest_executed_resolver()})
    {
        ModelStatus st;
        resolver->mark_committed(st, 500, 3);
        REQUIRE(st.committed_count.load() == 1);
        REQUIRE(st.committed_time.load() == 500);
        REQUIRE(st.latest_area.load() == 3);
        REQUIRE(st.generation.load() == 1);

        resolver->mark_committed(st, 900, 7);
        REQUIRE(st.committed_count.load() == 2);
        REQUIRE(st.committed_time.load() == 900);
        REQUIRE(st.latest_area.load() == 7);
    }
}

// ---------------------------------------------------------------------------
// ReadTargetResolver object: construction + write/read round-trip through the
// centralized status (no FMU required — the resolver skips non-FmuModel nodes)
// ---------------------------------------------------------------------------

namespace
{
    // Minimal Invocable that is NOT an FmuModel. The resolver must skip it,
    // so an "empty graph" resolver is testable without FMU resources.
    class StubInvocable : public ssp4sim::graph::Invocable
    {
    public:
        std::uint64_t invoke(ssp4sim::graph::StepData) override
        {
            return current_time;
        }
    };
}

TEST_CASE("ReadTargetResolver resolves unknown model as invalid", "[ReadTargetResolver]")
{
    ReadTargetResolver resolver({}, nullptr, default_config());

    StubInvocable unknown;
    ResolvedRead r = resolver.resolve(&unknown, 0, 900, 1100);
    REQUIRE_FALSE(r.valid);
}

TEST_CASE("ReadTargetResolver ignores non-FmuModel nodes", "[ReadTargetResolver]")
{
    StubInvocable stub;
    std::vector<ssp4sim::graph::Invocable *> models{&stub};

    // Should not crash: the resolver skips non-FmuModel invocables.
    ReadTargetResolver resolver(models, nullptr, default_config());

    ResolvedRead r = resolver.resolve(&stub, 0, 900, 1100);
    REQUIRE_FALSE(r.valid);
}

TEST_CASE("ReadTargetResolver mark_committed on unknown producer is a no-op", "[ReadTargetResolver]")
{
    ReadTargetResolver resolver({}, nullptr, default_config());
    StubInvocable stub;

    // Should not crash and should not throw.
    resolver.mark_committed(&stub, 1000, 0);
}

// ---------------------------------------------------------------------------
// Description: the resolver choice is injected at construction; the shell defaults to
//              the macro-step-start-time policy when none is supplied
// Rationale:   Enables pluggable read policy without FMU resources (the shell skips
//              non-FmuModel nodes, so only construction + no-crash is verifiable here).
// ---------------------------------------------------------------------------
TEST_CASE("ReadTargetResolver accepts an injected latest-executed resolver", "[ReadTargetResolver]")
{
    ReadTargetResolver resolver({}, latest_executed_resolver(), default_config());
    StubInvocable unknown;
    REQUIRE_FALSE(resolver.resolve(&unknown, 0, 900, 1100).valid);
}