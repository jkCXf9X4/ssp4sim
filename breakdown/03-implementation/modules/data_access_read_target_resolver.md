# Module: Data Access & Scheduling — Read-Target Resolver (implementation mockup)

<!-- Layer: 03-implementation -->
<!-- Status: Implementation mockup (revision 1). NOT implemented, NOT final. Mockup only —
     encodes design constraints, no decision implied. Split out from
     data_access_policy_architecture.md so the architecture doc stays concept-only.
     A REVISED, much-simplified external surface lives in v2 (see link below). -->
<!-- Sister docs:
   data_access_read_target_resolver_v2.md     (revision 2 — streamlined index/time interface)
   data_access_policy_architecture.md         (architecture reasoning, concept-level)
   data_access_and_scheduling_design.md       (mechanisms A–H)
   data_access_and_scheduling_problem_space.md (problem-only, solution-free) -->

## Purpose

This is the **revision 1** mockup of the read-target resolver. It is superseded in *surface*
by revision 2 — `data_access_read_target_resolver_v2.md` — which drastically simplifies the
external interface (construct against the models only; one read function returning index or
time by mode; access logic concealed from `copy_model_inputs`).

**This document remains the analysis anchor** for the UC-14 unlinked-read determinism
reasoning and the constraint mapping; v2 is the streamlined external contract for
integrating against `FmuModel` / `copy_model_inputs`. Where the two differ on interface,
v2 wins; where the two differ on determinism, this document's reasoning governs.

## Purpose

This is a concrete C++ sketch of the **read-target resolver** — a per-connection resolver
that, for each connection, evaluates *what is the latest / appropriate / valid read target*
(an area, resolved via a time) and whether the read is legal at this point in the schedule.
It has **access to the simulation graph** (obtained in its constructor), and through the
graph it reaches the connectors and connections of every model. It tracks model-per-model
runtime status **centrally**, so any executor or read path reaches the same frontier facts
without per-model bookkeeping scattered across callers.

It is the concrete form of design **C's** AccessWindow rule, composed with **B/G** (direct
index from a committed frontier), and a *reduction* of the pluggable access-policy
architecture (candidate I) — it is a component, not a strategy system. This mockup is the
**initial, streamlined version**: it deliberately keeps the surface small and generic, and
defers classification machinery (edge roles, scopes, joint-frame vectors) to later
revisions.

The sketch is written against the repo's existing types (`GraphExecutor`, `Invocable`,
`FmuModel`, `ConnectionInfo`, `SignalStorage`, `retrieve_model_inputs`), see
`lib/include/simulation/graph_executor/graph_executor.hpp`,
`lib/include/pre/3_simulation/elements/model_fmu.hpp`,
`lib/include/pre/3_simulation/elements/model_connection.{hpp,cpp}`. **Mockup only — not
implemented, not final, and no decision is implied.** It honours the non-nesting basis: the
resolver is resolved on a flat schedule; a future sub-executor, if adopted, must be a flat
projection of the same inputs and is never allowed to change this contract.

---

## Mockup — `ReadTargetResolver` (interface, constructor, usage)

### Graph access & centralized per-model status (the vision)

The constructor takes the simulation graph. It walks the graph **once** and derives three
tables that the rest of the system uses seamlessly:

| Table | Key | Value | Purpose |
|---|---|---|---|
| `status_` | producer `Invocable*` | `ModelStatus` | **Centralized per-model frontier** — the only mutable state (M1b) |
| `edges_` | target `Invocable*` | `Edge[]` | Per-target incoming connections, derived from `ConnectionInfo` at build |
| `owner_` | `SignalStorage*` | producer `Invocable*` | Reverse map storage → producer, so any read reaches its frontier "for free" |

Every connection in the system is thus reachable from the graph, and every connection's
frontier is the same central `ModelStatus` — this is what makes model-per-model tracking
"seamless": the write side (`mark_committed`) and the read side (`resolve`) both hit one
table, no per-model wiring.

```cpp
namespace ssp4sim::scheduling

// --- Centralized per-model runtime status ---------------------------------
// The ONLY mutable runtime state (M1b). Location is centralized in the resolver;
// updates go through one entry point (mark_committed); reads go through resolve().
struct ModelStatus
{
    std::atomic<uint64_t> committed_count = 0; // how many areas the producer has committed
    std::atomic<uint64_t> committed_time  = 0; // output time of the latest committed area
    std::atomic<uint64_t> latest_area     = 0; // physical slot of the latest committed area
};

// --- ResolverConfig -------------------------------------------------------
struct ResolverConfig
{
    bool    clamp_stale_shortfall = true;  // M1a: clamp reference to committed frontier, don't tear
    bool    check_staleness_signature = true; // M1c
    int32_t max_lookback = 0;      // D8 bound: lookback ≤ capacity − 2; 0 = derived from storages
};

// --- Edge — one incoming connection, derived from ConnectionInfo at build ---
// ConnectionInfo stays the wire type; Edge is the resolver's registry row.
// An `unlinked` Edge is data access WITHOUT an explicit graph/connection edge:
// the source is a producer the DAG does not order against this consumer. Such reads
// are stale-only by construction (no happens-before ⇒ no fresh), resolved straight to
// the committed frontier (see "Unlinked data access").
struct Edge
{
    ssp4sim::signal::SignalStorage *source;      // producer output storage
    uint32_t source_index;                       // index into the producer's output area
    DataAccessMode mode;                         // StartTime / EndTime / Latest / Index
    uint64_t delay;                              // from ConnectionInfo
    int64_t  time_offset;                        // from ConnectionInfo (only ever 0 today)
    Invocable *source_producer;                  // resolved via owner_ — the frontier owner
    bool unlinked = false;                       // no graph/connection edge → stale-only read
};

// --- Read target (what a connection resolves to) ---------------------------
enum class ReadState : uint8
{
    Ready,       // producer committed; reference time set; legal to look up and copy
    NotReady,    // producer has not committed yet this frame — skip copy (keep init)
};

struct ReadTarget
{
    ReadState state;
    ssp4sim::signal::SignalStorage *source;   // == edge.source (kept for the read loop)
    uint32_t  source_index;
    uint64_t  reference_time;                 // "what time to search for" — frontier-clamped
    uint64_t  generation;                     // status generation this was resolved against (M1c)

    // Sentinel: linked reads obtain the area via find_latest_valid_area(reference_time);
    // a concrete area value means the resolver already pinned it (unlinked → latest_area).
    static const std::size_t LIVE_LOOKUP = std::numeric_limits<std::size_t>::max();
    std::size_t area = LIVE_LOOKUP;           // pinned area, or LIVE_LOOKUP for linked reads
};
```

### The resolver

```cpp
class ReadTargetResolver
{
public:
    // Constructor — takes the SIMULATION GRAPH. Walks the graph once:
    //   - for each FmuModel, registers its connections as Edges (edges_)
    //   - creates one ModelStatus per producer (status_)
    //   - builds storage → producer ownership map (owner_)
    // Through graph_ the resolver keeps live access to every model's connectors
    // and connections for the life of the graph.
    ReadTargetResolver(ssp4sim::graph::GraphExecutor &graph, ResolverConfig cfg);

    // Write side — the ONLY way to advance a producer's frontier (executor calls this,
    // D17: after value bytes are fully written and visible). Advances the centralized
    // ModelStatus of `producer`.
    void mark_committed(Invocable *producer, uint64_t output_time, size_t area);

    // Read side — for one target model's incoming connections, compute the read target:
    //   "what is the latest / appropriate / valid time this connection may search at?"
    // Clamped to the source producer's committed frontier (M1a, D2/D7). Pure: no mutation.
    std::vector<ReadTarget> resolve(Invocable *target_model,
                                    uint64_t input_time, uint64_t step_start, uint64_t step_end);

    // Graph accessor — the resolver's reach into the graph (for connectors/connections).
    ssp4sim::graph::GraphExecutor &graph() const noexcept;

    // Build-time validation — fail-fast: lookback ≤ capacity−2, areas ≥ 2, index mode bounds.
    static void validate(ssp4sim::graph::GraphExecutor &graph, ResolverConfig cfg);

private:
    ssp4sim::graph::GraphExecutor &graph_;
    std::unordered_map<Invocable *, ModelStatus> status_;  // centralized per-model status
    std::unordered_map<Invocable *, std::vector<Edge>> edges_; // incoming connections per target
    std::unordered_map<SignalStorage *, Invocable *> owner_;   // storage → producer
    ResolverConfig cfg_;

    ReadTarget resolve_one(const Edge &e,
                           uint64_t input_time, uint64_t step_start, uint64_t step_end);
};
```

### Resolution rule (the core of `resolve_one`)

Per connection, with T = the time handle selected by `mode` (`StartTime` → `step_start`,
`EndTime` → `step_end`, `Index` → no time; in **v2** `Latest` resolves to the latest
committed area index directly — no time involved, no `input_time`), and then
`reference = T + time_offset − delay`:

1. **Index mode** → read the fixed slot directly (`README: fixed physical slot`, absolute
   semantics). State `Ready` if populated, else `NotReady`.
2. **Producer not yet committed** (`committed_count == 0`) → `NotReady` (first-commit /
   t=0 boundary, D2/D13). The central status makes this uniform across the whole graph.
3. **Producer committed** → clamp: `reference = min(reference, committed_time)` (M1a). The
   clamp *never tears* (D2) — at worst a fresh read degrades to the last committed area,
   which the read loop may treat as stale-but-legal.
4. **Unlinked edge** (`Edge.unlinked`) → skip any `T`-derived reference entirely; resolve
   `reference = committed_time` and set `source_area = ModelStatus.latest_area`: the read is
   **stale-only by construction**, never looks at the live head, and is a direct `get_item`
   with no scan (see below). This is both the deterministic choice and the cheapest read in
   the system.
5. Return `Ready` with the clamped `reference_time`. For unlinked reads, the caller then uses
   `latest_area` directly (step 4); for linked reads, `find_latest_valid_area(reference_time)`
   — **bounded by the committed frontier**, never the live write head (D7).

### Unlinked data access (reads to models with no explicit graph edge; UC-14)

A new requirement (use-case **UC-14** in `data_access_and_scheduling_problem_space.md`): a
model may need data from a producer with **no explicit node link** in the model graph — no
connection edge, so the DAG/wavefront carries no happens-before for it. The resolver still
must return **deterministic** data here, despite the missing link making the target
vulnerable to parallel-execution access phenomena.

**Where the risk is, precisely.** `find_latest_valid_area` scans `write_count − i for
i < capacity` and returns the newest slot with `timestamp ≤ requested`. `write_count` and the
slot contents **advance by the live write/append** (via the model's `post()` → `push`), not
by the commit frontier. So a read against the *live head* is vulnerable: if the producer's
append lands mid-batch, two runs with different thread interleavings see different areas —
exactly the D2-class race the resolver is built to prevent. `mark_committed()` advances the
`ModelStatus` frontier but does **not** stop the storage's write head from moving.

**Why the resolver's contract already makes it deterministic — *provided* the read is
frontier-bounded.** The clamp in step 3 (`reference = min(reference, committed_time)`) plus
step 2's `NotReady` gate means reads only ever reach a reference ≤ the producer's committed
frontier. But there is a subtlety: the *scan* in `find_latest_valid_area` reads the live ring
whose physical slots are being overwritten by the append — even with the right *logical*
timestamp, the physical copy could still race an in-progress slot write. So determinism for
unlinked reads needs two distinct guarantees to hold together:

1. **Scope of the rule.** For an edge that *is* in the graph, the schedule grants happened-
   before via the DAG; the frontier clamp is a safety net. For an **unlinked** read, there is
   no schedule ordering — the read must therefore be **stale-only by construction**: it may
   only ever target the committed frontier (`reference = committed_time`), never "what the
   live head now shows". A fresh/serial/Seidel-style read of an unlinked producer is simply
   **not legal** (there is no happen-before to make it deterministic).
2. **Physical stability while copying.** Even a committed *area* can be physically overwritten
   by the *next* append (ring wrap). The read must copy from a slot that no in-batch append
   will touch — i.e. the copy must be issued against the committed area of a *frozen* storage
   segment, or (pragmatically) the read is only issued where the append and the copy are
   mutually excluded or where the producer commits with the same release/acquire ordering
   (D17) so the area stays valid until the consumer's acquire.

The cleanest way to *state* the unlinked-read contract, without over-designing the initial
implementation:

> **Unlinked reads are always stale (lookback = 0 against the committed frontier, never the
> live head), and they are only legal through the resolver — which refuses to serve a
> not-yet-committed or mid-append view.** If a caller needs "fresh" from an unlinked
> producer, that is a graph-construction requirement (add the edge), not a resolver capability.

**Design consequences for the initial mockup.**

- The `Edge` for an unlinked read gets a marker (e.g. `Edge.unlinked = true`), and
  `resolve_one` enforces: for unlinked edges, `reference = min(reference, committed_time)`
  **and additionally `reference = committed_time` when the producer has *any* commit at all**
  (never look at the head). Concretely the reference for an unlinked stale read must be the
  **last committed time as of the batch boundary**, not "now".
- `ModelStatus.latest_area` is exercised for these reads: instead of a `find_latest_valid_area`
  scan (which walks toward the live head), the unlinked read can go **straight to the recorded
  committed area** `owner_[storage] → ModelStatus.latest_area`, doing a direct `get_item` with
  **no scan at all**. This both removes the determinism exposure and makes unlinked reads the
  cheapest reads in the system.
- **Physical-stability enforcement is a storage concern, and the mockup states it as a
  precondition**: the committed area an unlinked read targets must be *append-free* while the
  consumer copies. This is the existing D17 release/acquire obligation (value bytes fully
  written → `mark_committed` release-store → consumer acquire-load then copy), and it is
  exactly where a missing graph link cannot manufacture security: the executor must still
  commit only after the producer's `post()`/append is done, so the copy never races a
  physical write. Distributed parallel appends to the same storage (multiple producers)
  remain out of scope for this initial version, as today.

**Net.** The resolver design as sketched already contains the right *shape* — a central
`ModelStatus` per producer plus a clamped reference — and the unlinked-read ask simply
sharpens it: unlinked reads are *mandatorily stale*, resolved straight to the recorded
committed area (cheap + deterministic), and the physical-stability guarantee is delegated to
the D17 commit ordering the executor must already honor. The vulnerability the concern calls
out disappears precisely because reads never fall back to scanning the live write head.

### Read-loop usage (replaces the policy part of `retrieve_model_inputs`)

```cpp
// Hot path — copies a target model's inputs from the resolver's read targets.
// Run once per model pre(); unlinked reads go straight to the committed area (no scan);
// linked reads do one frontier-bounded lookup. No mode logic here.
void copy_model_inputs(ReadTargetResolver &res, FmuModel *target, int target_area,
                       uint64_t input_time, uint64_t step_start, uint64_t step_end)
{
    for (const ReadTarget &rt : res.resolve(target, input_time, step_start, step_end))
    {
        if (rt.state != ReadState::Ready) continue;         // NotReady → keep init / skip
        size_t source_area = rt.area;                       // unlinked: latest_area (no scan);
        if (rt.area == ReadTarget::LIVE_LOOKUP)             // linked: frontier-bounded lookup
            if (!rt.source->find_latest_valid_area(rt.reference_time, source_area)) continue;
        std::byte *src = rt.source->get_item(source_area, rt.source_index);
        copy_value(src, target, target_area, rt.source_index); // type-aware copy (D15)
    }
}
```

### Executor usage (the only writer of the centralized status)

```cpp
// --- pure serial Jacobi ------------------------------------------------
void run_jacobi(GraphExecutor &graph, ReadTargetResolver &res)
{
    for (uint64_t step = 0; step < steps; ++step)
    {
        for (Invocable *model : graph.nodes)                 // produce all
        {
            model->invoke(StepData(...));
            // output area was written by the model; commit it to the central frontier
            res.mark_committed(model, model->current_time, latest_output_area(model));
        }                                                    // barrier per step
    }                                                        // consumers read next step
}
```

### Notes against design constraints

| Constraint | Where the mockup honours it |
|---|---|
| Read resolves to an area/time, read loop only copies | `ReadTarget.reference_time` + `find_latest_valid_area`; mode logic lives in the resolver, not the read loop |
| Phase adjusts rules, frontier gates targets (M1a) | `reference = min(reference, committed_time)` clamp inside `resolve_one` |
| Centralized single mutable truth (M1b) | one `status_` table; only `mark_committed` writes it; `resolve` is pure |
| Staleness signature (M1c) | `generation` on `ModelStatus`/`ReadTarget`; executor compares vs. previous build |
| Must not invent happens-before | `NotReady` if the producer's `committed_count == 0`; freshness follows commits only |
| **Unlinked reads are stale-only (no graph edge)** | unlinked `Edge` reads resolve to `committed_time`/`latest_area` directly, never the live head; request "fresh" ⇒ graph fault, not a lookback |
| Non-nesting basis | `Edge.source_producer` is a plain flat pointer resolved via `owner_`; no executor tree |
| Graph reach for connectors/connections | constructor takes `GraphExecutor`; edges derive from each model's `connections` (`unlinked` edges populate `status_`/`owner_` without a `connections` entry) |
| Lookback bound (D8) | `validate()` checks `lookback ≤ capacity − 2`, `areas ≥ 2` |
| Write side stays in executors | `mark_committed` is executor-called; the resolver never advances frontiers itself |
| Type-aware copy (D15) | `copy_value` in the read loop, untouched by the resolver |
| Physical stability while copying (D17) | commit (release-store) is issued after the producer's `post()`/append; the unlinked read acquires then copies, so the target area is not mid-append |

### Intentionally deferred (keeps the initial version clean)

The following machinery from the full design space is **not** in this initial mockup, and is
only added when a concrete need appears:

- **`AccessPlan` / plan-execute split** — no cached plan; `resolve()` runs per model
  `pre()`. The hot path is still tiny (a linear pass over a model's connections with one
  clamp + one frontier-bounded lookup each). Plan caching is the escape hatch when
  profiling demands it, and it changes no signature: it makes `resolve()` cheaper, not
  different.
- **`EdgeRole` (Stale/Fresh/Delayed/FrameBound) and scope tags** — the initial version
  expresses staleness/freshness purely through `mode` + `delay` + the commit frontier. Role
  classification is additive later.
- **Joint-frame (F/D16) version vectors** — no cross-producer consistency yet; a consumer
  that differences several producers reads each at its own frontier. Documented as
  unsupported for now.
- **Per-scope frontiers / nesting (M2)** — flat schedule only, one frontier per producer.
- **Unlinked *fresh* access** — unlinked reads are stale-only by construction (no graph edge ⇒
  no happens-before ⇒ fresh is not legal). If a model needs fresh data from outside its DAG,
  that is a graph-construction requirement (add the edge), left explicit as a build fault in
  `validate()`.

---

## Future revisions

This document is a mockup and is expected to be superseded by whatever design is eventually
chosen. It is kept here so the constraint mapping (graph-injected constructor, centralized
per-model status, frontier-clamped read time, **unlinked reads stale-only**, M1a clamp, M1b
single mutable truth, M1c staleness signature, no invented happens-before, non-nesting
basis, lookback bound D8, write-side-in-executor, D15 type-aware copy, D17 physical
stability) has a concrete, reviewable anchor. Changes to the architecture doc that touch
those constraints must be mirrored here.

Anticipated growth path (each is deferred by design, not forgotten):

1. **Plan caching** — `AccountPlan`/build-once-per-phase when profiling shows `resolve()`
   per model `pre()` is worth hoisting; signatures stay, only the inner work changes.
2. **Edge roles & scopes** — `EdgeRole` (Stale/Fresh/Delayed/FrameBound) + scope tags for
   UC-3/UC-5 when in-sweep-fresh or SCC iteration semantics are actually needed.
3. **Joint freshness** — per-consumer version vectors (F/D16) for consumers that difference
   multiple producers.
4. **Nesting** — per-scope frontiers (M2) only if a sub-executor ever hosts a genuinely
   different algorithm; never a change to the resolver contract.
