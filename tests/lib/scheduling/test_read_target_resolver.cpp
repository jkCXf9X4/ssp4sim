#include "scheduling/read_target_resolver.hpp"
#include "scheduling/read_target_core.hpp"

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
using ssp4sim::scheduling::detail::resolve_edge;
using ssp4sim::graph::ConnectionInfo;
using ssp4sim::graph::DataAccessMode;
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
TEST_CASE("resolve_edge Latest returns the latest committed index", "[ReadTargetResolver]")
{
    Edge e; // mode defaults to Latest
    ModelStatus st;
    set_committed(st, 500, 3);

    ResolvedRead r = resolve_edge(e, st, default_config(), 900, 1100);

    REQUIRE(r.valid);
    REQUIRE(r.is_area);
    REQUIRE(r.area == 3); // latest_area, regardless of requested step span
    REQUIRE(r.generation == 1);
}

TEST_CASE("resolve_edge Latest is invalid before first commit", "[ReadTargetResolver]")
{
    ModelStatus st; // committed_count == 0
    Edge e; // Latest

    ResolvedRead r = resolve_edge(e, st, default_config(), 900, 1100);
    REQUIRE_FALSE(r.valid);
}

// ---------------------------------------------------------------------------
// Description: Latest ignores delay/time_offset (index-domain address, not a time search)
// Rationale:   A delay/lookback on the latest index is future work (D8/B); the initial
//              Latest is simply "the newest committed area"
// ---------------------------------------------------------------------------
TEST_CASE("resolve_edge Latest ignores delay and time_offset", "[ReadTargetResolver]")
{
    ModelStatus st;
    set_committed(st, 10'000, 5);

    Edge e; e.mode = DataAccessMode::Latest; e.delay = 2; e.time_offset = 5;
    ResolvedRead r = resolve_edge(e, st, default_config(), 900, 1100);
    REQUIRE(r.valid);
    REQUIRE(r.is_area);
    REQUIRE(r.area == 5);
}

// ---------------------------------------------------------------------------
// Description: StartTime / EndTime select the step handles
// Rationale:   Feedback edges sample start-of-step; delayed edges sample end-of-step
// ---------------------------------------------------------------------------
TEST_CASE("resolve_edge StartTime and EndTime select step handles", "[ReadTargetResolver]")
{
    ModelStatus st;
    set_committed(st, 10'000, 1);

    Edge start; start.mode = DataAccessMode::StartTime;
    ResolvedRead r1 = resolve_edge(start, st, default_config(), 100, 200);
    REQUIRE(r1.valid);
    REQUIRE_FALSE(r1.is_area);
    REQUIRE(r1.time == 100);

    Edge end; end.mode = DataAccessMode::EndTime;
    ResolvedRead r2 = resolve_edge(end, st, default_config(), 100, 200);
    REQUIRE(r2.valid);
    REQUIRE_FALSE(r2.is_area);
    REQUIRE(r2.time == 200);
}

// ---------------------------------------------------------------------------
// Description: delay and time_offset shift the (time-domain) StartTime/EndTime reference
// Rationale:   Transport/delay edges and the (currently latent) time_offset knob
// ---------------------------------------------------------------------------
TEST_CASE("resolve_edge delay and time_offset shift StartTime/EndTime", "[ReadTargetResolver]")
{
    ModelStatus st;
    set_committed(st, 10'000, 1);

    Edge delayed; delayed.mode = DataAccessMode::StartTime; delayed.delay = 3;
    ResolvedRead r = resolve_edge(delayed, st, default_config(), 100, 200);
    REQUIRE(r.time == 97); // 100 − 3

    Edge offset; offset.mode = DataAccessMode::EndTime; offset.time_offset = -10;
    ResolvedRead r2 = resolve_edge(offset, st, default_config(), 100, 200);
    REQUIRE(r2.time == 190); // 200 − 10

    Edge both; both.mode = DataAccessMode::StartTime; both.delay = 2; both.time_offset = 5;
    ResolvedRead r3 = resolve_edge(both, st, default_config(), 1000, 1100);
    REQUIRE(r3.time == 1003); // 1000 + 5 − 2
}

// ---------------------------------------------------------------------------
// Description: negative reference is floored at zero (no unsigned underflow, D8)
// Rationale:   Int64 → uint64 cast must never wrap; D2-class first-commit underflow
// ---------------------------------------------------------------------------
TEST_CASE("resolve_edge floors negative reference at zero", "[ReadTargetResolver]")
{
    ModelStatus st;
    set_committed(st, 10'000, 1);

    Edge e; e.mode = DataAccessMode::StartTime; e.delay = 500;
    ResolvedRead r = resolve_edge(e, st, default_config(), 100, 200);
    REQUIRE(r.valid);
    REQUIRE(r.time == 0);
}

// ---------------------------------------------------------------------------
// Description: clamping is only reachable on time modes; can also be disabled
// Rationale:   Clamp (M1a) is the determinism contract for time-domain sampling
// ---------------------------------------------------------------------------
TEST_CASE("resolve_edge StartTime clamps to the committed frontier", "[ReadTargetResolver]")
{
    ModelStatus st;
    set_committed(st, 500, 1);

    Edge e; e.mode = DataAccessMode::StartTime;
    ResolvedRead r = resolve_edge(e, st, default_config(), 1000, 1100);
    REQUIRE(r.valid);
    REQUIRE(r.time == 500); // clamped to committed frontier

    ResolverConfig cfg; cfg.clamp_stale_shortfall = false;
    ResolvedRead r2 = resolve_edge(e, st, cfg, 1000, 1100);
    REQUIRE(r2.valid);
    REQUIRE(r2.time == 1000); // raw, no clamp
}

// ---------------------------------------------------------------------------
// Description: Index mode returns the fixed physical slot as a direct area
// Rationale:   Fixed-slot addressing bypasses time lookup entirely
// ---------------------------------------------------------------------------
TEST_CASE("resolve_edge Index mode returns the fixed slot", "[ReadTargetResolver]")
{
    ModelStatus st;
    set_committed(st, 500, 3);

    Edge e; e.mode = DataAccessMode::Index; e.fixed_index = 7;
    ResolvedRead r = resolve_edge(e, st, default_config(), 900, 1100);
    REQUIRE(r.valid);
    REQUIRE(r.is_area);
    REQUIRE(r.area == 7);
}

// ---------------------------------------------------------------------------
// Description: unlinked reads are stale-only, direct to the committed area (UC-14)
// Rationale:   No graph edge ⇒ no happens-before ⇒ never the live head
// ---------------------------------------------------------------------------
TEST_CASE("resolve_edge unlinked read resolves to committed area only", "[ReadTargetResolver]")
{
    ModelStatus st;
    set_committed(st, 500, 3);

    Edge e; e.mode = DataAccessMode::StartTime; e.unlinked = true; e.delay = 100;
    ResolvedRead r = resolve_edge(e, st, default_config(), 900, 1100);
    REQUIRE(r.valid);
    REQUIRE(r.is_area);
    REQUIRE(r.area == 3); // latest_area, regardless of requested time/delay
    REQUIRE(r.generation == 1);
}

TEST_CASE("resolve_edge unlinked read invalid before first commit", "[ReadTargetResolver]")
{
    ModelStatus st;
    Edge e; e.mode = DataAccessMode::Latest; e.unlinked = true;
    ResolvedRead r = resolve_edge(e, st, default_config(), 900, 1100);
    REQUIRE_FALSE(r.valid);
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
    ReadTargetResolver resolver({}, default_config());

    StubInvocable unknown;
    ResolvedRead r = resolver.resolve(&unknown, 0, 900, 1100);
    REQUIRE_FALSE(r.valid);
}

TEST_CASE("ReadTargetResolver ignores non-FmuModel nodes", "[ReadTargetResolver]")
{
    StubInvocable stub;
    std::vector<ssp4sim::graph::Invocable *> models{&stub};

    // Should not crash: the resolver skips non-FmuModel invocables.
    ReadTargetResolver resolver(models, default_config());

    ResolvedRead r = resolver.resolve(&stub, 0, 900, 1100);
    REQUIRE_FALSE(r.valid);
}

TEST_CASE("ReadTargetResolver mark_committed on unknown producer is a no-op", "[ReadTargetResolver]")
{
    ReadTargetResolver resolver({}, default_config());
    StubInvocable stub;

    // Should not crash and should not throw.
    resolver.mark_committed(&stub, 1000, 0);
}