#include "resolver/read_resolver.hpp"
#include "resolver/read_target_core.hpp"

#include "pre/3_simulation_graph/elements/model_connection.hpp"

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <memory>
#include <vector>

using ssp4sim::scheduling::DataAccessResolver;
using ssp4sim::scheduling::ResolvedRead;
using ssp4sim::scheduling::AccessMode;
using ssp4sim::scheduling::EdgeAccessRules;
using ssp4sim::scheduling::detail::ModelStatus;
using ssp4sim::scheduling::detail::resolve_edge;
using ssp4sim::graph::Invocable;

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
}

// ---------------------------------------------------------------------------
// Description: Latest mode resolves to the latest committed area index directly —
//              no time, no scan
// Rationale:   "Latest" means newest committed index (zero-order hold); the index is
//              the deterministic anchor, never the live head (M1a/D2/D7)
// ---------------------------------------------------------------------------
TEST_CASE("resolve_edge Latest returns the latest committed index", "[ReadResolver]")
{
    EdgeAccessRules e; // mode defaults to Latest
    ModelStatus st;
    set_committed(st, 500, 3);

    ResolvedRead r = resolve_edge(e, st, 900, 1100);

    REQUIRE(r.valid);
    REQUIRE(r.is_area);
    REQUIRE(r.area == 3); // latest_area, regardless of requested step span
    REQUIRE(r.write_counter == 1);
}

// ---------------------------------------------------------------------------
// Description: Latest is invalid before any commit
// Rationale:   No committed frontirer => nothing to read (D2/D13)
// ---------------------------------------------------------------------------
TEST_CASE("resolve_edge Latest is invalid before first commit", "[ReadResolver]")
{
    EdgeAccessRules e; // Latest
    ModelStatus st;

    ResolvedRead r = resolve_edge(e, st, 900, 1100);
    REQUIRE_FALSE(r.valid);
}

// ---------------------------------------------------------------------------
// Description: Latest ignores delay/time_offset (index-domain address, not a time search)
// Rationale:   A delay/lookback on the latest index is future work; Latest is simply
//              "the newest committed area"
// ---------------------------------------------------------------------------
TEST_CASE("resolve_edge Latest ignores delay and time_offset", "[ReadResolver]")
{
    ModelStatus st;
    set_committed(st, 10'000, 5);

    EdgeAccessRules e; e.mode = AccessMode::Latest; e.delay = 2; e.time_offset = 5;
    ResolvedRead r = resolve_edge(e, st, 900, 1100);
    REQUIRE(r.valid);
    REQUIRE(r.is_area);
    REQUIRE(r.area == 5);
}

// ---------------------------------------------------------------------------
// Description: StartTime / EndTime select the step handles
// Rationale:   Feedback edges sample start-of-step; delayed edges sample end-of-step
// ---------------------------------------------------------------------------
TEST_CASE("resolve_edge StartTime/EndTime select step handles", "[ReadResolver]")
{
    ModelStatus st;
    set_committed(st, 10'000, 1);

    EdgeAccessRules start; start.mode = AccessMode::StartTime;
    ResolvedRead r1 = resolve_edge(start, st, 100, 200);
    REQUIRE(r1.valid);
    REQUIRE_FALSE(r1.is_area);
    REQUIRE(r1.time == 100);

    EdgeAccessRules end; end.mode = AccessMode::EndTime;
    ResolvedRead r2 = resolve_edge(end, st, 100, 200);
    REQUIRE(r2.valid);
    REQUIRE_FALSE(r2.is_area);
    REQUIRE(r2.time == 200);
}

// ---------------------------------------------------------------------------
// Description: delay and time_offset shift the (time-domain) StartTime/EndTime reference
// Rationale:   Transport/delay edges and the (currently latent) time_offset knob
// ---------------------------------------------------------------------------
TEST_CASE("resolve_edge shifts by delay and time_offset", "[ReadResolver]")
{
    ModelStatus st;
    set_committed(st, 10'000, 1);

    EdgeAccessRules delayed; delayed.mode = AccessMode::StartTime; delayed.delay = 3;
    ResolvedRead r = resolve_edge(delayed, st, 100, 200);
    REQUIRE(r.time == 97); // 100 − 3

    EdgeAccessRules offset; offset.mode = AccessMode::EndTime; offset.time_offset = -10;
    ResolvedRead r2 = resolve_edge(offset, st, 100, 200);
    REQUIRE(r2.time == 190); // 200 − 10

    EdgeAccessRules both; both.mode = AccessMode::StartTime; both.delay = 2; both.time_offset = 5;
    ResolvedRead r3 = resolve_edge(both, st, 1000, 1100);
    REQUIRE(r3.time == 1003); // 1000 + 5 − 2
}

// ---------------------------------------------------------------------------
// Description: negative reference is floored at zero (no unsigned underflow, D8)
// Rationale:   Int64 → uint64 cast must never wrap; D2-class first-commit underflow
// ---------------------------------------------------------------------------
TEST_CASE("resolve_edge floors negative reference at zero", "[ReadResolver]")
{
    ModelStatus st;
    set_committed(st, 10'000, 1);

    EdgeAccessRules e; e.mode = AccessMode::StartTime; e.delay = 500;
    ResolvedRead r = resolve_edge(e, st, 100, 200);
    REQUIRE(r.valid);
    REQUIRE(r.time == 0);
}

// ---------------------------------------------------------------------------
// Description: clamping the reference to the committed frontier (M1a)
// Rationale:   Clamp is the determinism contract for time-domain sampling
// ---------------------------------------------------------------------------
TEST_CASE("resolve_edge clamps to the committed frontier", "[ReadResolver]")
{
    ModelStatus st;
    set_committed(st, 500, 1);

    EdgeAccessRules e; e.mode = AccessMode::StartTime;
    ResolvedRead r = resolve_edge(e, st, 1000, 1100);
    REQUIRE(r.valid);
    REQUIRE(r.time == 500); // clamped to committed frontier
}

// ---------------------------------------------------------------------------
// Description: Index mode returns the fixed physical slot as a direct area
// Rationale:   Fixed-slot addressing bypasses time lookup entirely
// ---------------------------------------------------------------------------
TEST_CASE("resolve_edge Index mode returns the fixed slot", "[ReadResolver]")
{
    ModelStatus st;
    set_committed(st, 500, 3);

    EdgeAccessRules e; e.mode = AccessMode::Index; e.fixed_index = 7;

    ResolvedRead r = resolve_edge(e, st, 900, 1100);
    REQUIRE(r.valid);
    REQUIRE(r.is_area);
    REQUIRE(r.area == 7);
}

// ---------------------------------------------------------------------------
// Description: unlinked (Latest-pinned) reads are stale-only, direct to the
//              committed area (UC-14)
// Rationale:   No graph edge ⇒ no happens-before ⇒ never the live head
// ---------------------------------------------------------------------------
TEST_CASE("resolve_edge unlinked read resolves to committed area only", "[ReadResolver]")
{
    ModelStatus st;
    set_committed(st, 500, 3);

    EdgeAccessRules e; e.mode = AccessMode::Latest; e.delay = 100;
    ResolvedRead r = resolve_edge(e, st, 900, 1100);
    REQUIRE(r.valid);
    REQUIRE(r.is_area);
    REQUIRE(r.area == 3); // latest_area, regardless of requested time/delay
    REQUIRE(r.write_counter == 1);

    ModelStatus unconsumed;
    REQUIRE_FALSE(resolve_edge(e, unconsumed, 900, 1100).valid);
}

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

// ---------------------------------------------------------------------------
// DataAccessResolver shell: construction + no-crash semantics without FMU
// resources (non-FmuModel nodes are skipped and unknown ids resolve invalid).
// ---------------------------------------------------------------------------

TEST_CASE("DataAccessResolver resolves unknown model as invalid", "[ReadResolver]")
{
    DataAccessResolver resolver(std::vector<Invocable *>{});

    StubInvocable unknown;
    ResolvedRead r = resolver.resolve(unknown.id, 0, 900, 1100);
    REQUIRE_FALSE(r.valid);
}

TEST_CASE("DataAccessResolver ignores non-FmuModel nodes", "[ReadResolver]")
{
    StubInvocable stub;
    std::vector<Invocable *> models{&stub};

    // Should not crash: the resolver skips non-FmuModel invocables.
    DataAccessResolver resolver(models);

    ResolvedRead r = resolver.resolve(stub.id, 0, 900, 1100);
    REQUIRE_FALSE(r.valid);
}

TEST_CASE("DataAccessResolver mark_committed on unknown producer is a no-op", "[ReadResolver]")
{
    DataAccessResolver resolver(std::vector<Invocable *>{});
    StubInvocable stub;

    // Should not crash and should not throw.
    resolver.mark_committed(stub.id, 1000, 0);
}