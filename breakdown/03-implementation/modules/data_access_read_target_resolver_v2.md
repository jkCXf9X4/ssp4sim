# Module: Data Access & Scheduling — Read-Target Resolver v2 (streamlined interface)

<!-- Layer: 03-implementation -->
<!-- Status: IMPLEMENTED (initial version) — lib/include/scheduling/read_target_resolver.{hpp,cpp}
     + tests/lib/scheduling/test_read_target_resolver.cpp. Tests green (11072 assertions /
     128 test cases suite-wide; resolver: 26 assertions / 12 cases). Still NOT a design decision;
     the object-level FmuModel integration (copy_model_inputs wiring) is a follow-up. -->
<!-- Sister docs:
   data_access_read_target_resolver.md          (v1 — full interface incl. UC-14 analysis)
   data_access_policy_architecture.md           (architecture reasoning, concept-level)
   data_access_and_scheduling_design.md         (mechanisms A–H)
   data_access_and_scheduling_problem_space.md  (problem-only, solution-free; UC) -->

## Purpose

Revision 2 of the **read-target resolver** mockup. The goal of this revision is a **much
simpler external interface** than v1:

- **Construct only against the graph models** (`Invocable*` set), not against the whole
  `GraphExecutor`. The constructor walks the models once, derives the centralized per-model
  status and the per-target connection registry from them, and that is it.
- **Expose exactly one read function** that returns, for one connection of one model,
  **either the area index or the viable time** *depending on `mode`* — the caller does not
  interpret modes, delays, offsets, unlinked markers, or frontiers.
- This makes `copy_model_inputs` **trivial** and **conceals the access logic heavily**: the
  model layer asks the resolver "what do I read for connection *i*" and either gets a pinned
  index (Index mode / unlinked stale) or a viable, frontier-clamped time (time modes).

The determinism contract is unchanged from v1 (frontier-clamped, no live-head reads,
unlinked reads stale-only, single committed-watermark truth). Only the **external surface**
is simplified; the logic that was spread across the caller is now inside the resolver.

> **Implemented** in `lib/include/scheduling/read_target_resolver.{hpp,cpp}` (namespace
> `ssp4sim::scheduling`; internal resolution core in `ssp4sim::scheduling::detail`).
> Both the mockup sketches below and the implementation agree; where this document's
> pseudo-code and the code differ, the code wins.

## Implementation notes (initial version)

- **File split** — the pure, storage-free resolution facts live in their own file pair so
  they are independently unit-testable and the public resolver header stays a thin opaque
  class. Each read policy is split into its own file pair for readability. Dependency
  direction: the policy/interface **depends on** the public resolver header and the facts
  core for their value types; the resolver header **never drags any of them in** — it only
  forward-declares `detail::ReadResolver` (the shell stores and forwards the pointer without
  dereferencing). All files are linked only into the resolver's `.cpp` and the tests:
  | File | Contains | Depends on |
  |---|---|---|
  | `lib/include/scheduling/read_target_resolver.hpp` | `ResolvedRead`, `ResolverConfig`, `ReadTargetResolver` class, forward-declared `detail::ReadResolver` | only `Invocable` / `FmuModel` / `SignalStorage` — NOT the policy/core |
  | `lib/include/scheduling/read_target_core.{hpp,cpp}` | `detail::{AccessMode, Edge, ModelStatus, edge_from, copy_connection}` | `read_target_resolver.hpp` (public types), `ConnectionInfo`, `Invocable` |
  | `lib/include/scheduling/read_resolver.hpp` + `.cpp` | `detail::ReadResolver` (abstract interface) + shared `detail::commit_frontier` | `read_target_resolver.hpp`, `read_target_core.hpp` |
  | `lib/include/scheduling/read_resolver_latest_executed.{hpp,cpp}` | `detail::LatestExecutedResolver` + `detail::latest_executed_resolver()` | `read_resolver.hpp` (transitively the facts) |
  | `lib/include/scheduling/read_resolver_macro_step_start_time.{hpp,cpp}` | `detail::MacroStepStartTimeResolver` + `detail::macro_step_start_time_resolver()` | `read_resolver.hpp` (transitively the facts) |
  | `lib/include/scheduling/read_target_resolver.cpp` | constructor, `mark_committed`, `resolve`, `copy_model_inputs` (opaque `State`) | links the facts core + both concrete resolver headers |
- **Pluggable read policy.** The old mode switch lived in the free function
  `detail::resolve_edge`; it is now `detail::ReadResolver`, an abstract interface with two
  methods — `mark_committed(ModelStatus&, output_time, area)` and
  `resolve(Edge, ModelStatus, ResolverConfig, step_start, step_end)`. The `ReadTargetResolver`
  shell is constructed with `detail::ReadResolver *resolver` (borrowed, not owned; `nullptr`
  selects the default `detail::macro_step_start_time_resolver()`) and forwards **both** the
  write side (`mark_committed`) and the read side (`resolve`) to it. New policies are added
  by implementing the interface in a new file pair, not by touching the shell or the
  edge-fact model.
- **The two initial resolvers.**
  - `detail::LatestExecutedResolver` (read_resolver_latest_executed.{hpp,cpp}) — every
    connection reads the producer's newest committed area (zero-order hold), never the live
    head. This is the choice to enable when a consumer must always see "the latest executed
    value".
  - `detail::MacroStepStartTimeResolver` (read_resolver_macro_step_start_time.{hpp,cpp}) —
    the graph default. Wired edges sample at the macro step start handle (shifted by
    `delay`/`time_offset`, clamped to the committed frontier), unlinked / Latest edges read
    the newest committed area, Index edges keep the fixed slot. It reproduces the
    pre-refactor `resolve_edge` behaviour exactly.
  Both are stateless singletons shared via `detail::macro_step_start_time_resolver()` and
  `detail::latest_executed_resolver()`; their `mark_committed` both delegate to the shared
  `detail::commit_frontier`; all per-producer frontier lives in `ModelStatus`.
- `detail::ModelStatus` uses `std::atomic<std::uint64_t>` fields; it is held in the
  resolver behind `std::unique_ptr` (atomics are non-movable, so it cannot be a map *value*).
- `mark_committed` is a no-op for unregistered producers (graceful; returns, matching the
  "unknown model ⇒ invalid read" behaviour of `resolve`).
- Index-mode resolution returns the fixed slot as an area; the *populated* gate is applied
  by the resolver shell (the pure resolvers cannot reach the storage).
- Unlinked reads (UC-14) are auto-detected at construction: a connection whose
  `source_storage` is not owned by any registered model's `output_area` is marked
  `unlinked` and resolves stale-only to the committed area.
- All resolver-private *tables* (`State::edges/status/owner/cfg`) are behind the opaque
  `State*` completed only in the `.cpp` — the header exposes zero private layout.
- **`ConnectionInfo` no longer carries sampling policy.** `DataAccessMode`, `mode`,
  `time_offset`, and `fixed_index` were removed from `ConnectionInfo`; the retired
  `retrieve_model_inputs` is gone. Sampling policy now lives entirely in the chosen
  `detail::ReadResolver` acting on the resolver's `detail::Edge` facts (default
  `StartTime` for every wired edge, the behaviour the builder previously pinned). The graph
  builder no longer sets any mode/offset; it wires only facts (`delay`, `is_feedthrough`,
  type/size/indices).
- **Copy ownership.** `copy_model_inputs(FmuModel*, …)` replaces `retrieve_model_inputs`.
  `FmuModel::pre()` and `direct_feedthrough()` call it when a resolver is attached, and
  `FmuModel::post()` calls `mark_committed` (D17 release-store) so downstream reads resolve
  against committed output. The resolver is built in `build_simulation_graph` right after
  model wiring and owned by `SimulationPipelineResult`; every `FmuModel` borrows it via
  `access_resolver`.
- **In GraphExecutor/Simulation tests** the read path is exercised via the resolvers'
  `resolve` + `detail::copy_connection` (FMU-free); full FMU integration runs through the
  Python suite.

---

## The whole public surface

```cpp
namespace ssp4sim::scheduling

// --- Result of resolving one connection -----------------------------------
// Exactly one of `area` / `time` is meaningful, selected by `is_area`.
// The caller never inspects connections, modes, delays, offsets, or frontiers.
struct ResolvedRead
{
    bool     valid;      // false → producer not committed yet (keep init / skip this frame)
    bool     is_area;    // true  → `area` is the storage slot to read (no search at all)
                         // false → `time` is the viable, frontier-clamped reference (search ≤ it)
    size_t   area;       // valid when is_area && valid
    uint64_t time;       // valid when !is_area && valid
    uint64_t generation; // frontier generation this was resolved against (M1c, optional check)
};

class ReadTargetResolver
{
public:
    // Constructor — takes the graph MODELS and, optionally, the read policy:
    //   - for each FmuModel, registers its connections as edges (edges_)
    //   - creates one ModelStatus per producer (status_)
    //   - builds storage → producer ownership (owner_)
    //   - optional: registers "unlinked" reads (UC-14) supplied here, no runtime API.
    // `resolver` is the injected detail::ReadResolver (borrowed; nullptr → the default
    // detail::macro_step_start_time_resolver()). Both mark_committed and resolve are
    // forwarded to it.
    ReadTargetResolver(std::vector<ssp4sim::graph::Invocable *> models,
                       detail::ReadResolver *resolver = nullptr,
                       ResolverConfig cfg = {});

    // The ONLY write-side entry — advances a producer's committed frontier (D17:
    // release-store AFTER value bytes are visible; never called from push()). Forwarded
    // to the injected resolver.
    void mark_committed(ssp4sim::graph::Invocable *producer,
                        uint64_t output_time, size_t area);

    // THE one read function. For target `model`, incoming connection `connection_idx`,
    // return the index to read OR the viable time to search, per the injected resolver.
    // No `input_time`: the Latest/area policies resolve to the latest committed index.
    ResolvedRead resolve(ssp4sim::graph::Invocable *model, size_t connection_idx,
                         uint64_t step_start, uint64_t step_end);

    // Everything else is private.
};
```

That is the entire interface: **one constructor (models + policy), one write method, one
read method.** No `BuildPlan`, no `AccessPlan`, no `graph()` accessor, no per-read
interpretation left to the caller.

---

## What the constructor precomputes (walking the models)

| Private table | Key | Value | Derived from |
|---|---|---|---|
| `status_` | producer `Invocable*` | `ModelStatus` | each model's `output_area` (centralized committed frontier, M1b) |
| `edges_` | target `Invocable*` | `Edge[]` in *connections order* | each `FmuModel::connections` |
| `owner_` | `SignalStorage*` | producer `Invocable*` | each model's `output_area` (reverse map) |

`Edge` is the resolver's *internal* registry row (ConnectionInfo stays the wire type):

```cpp
struct Edge   // private
{
    ssp4sim::signal::SignalStorage *source;   // producer output storage
    uint32_t  source_index;
    ssp4sim::scheduling::detail::AccessMode mode;   // StartTime / EndTime / Latest / Index
    uint64_t  delay;
    int64_t   time_offset;
    int64_t   fixed_index;    // Index mode
    ssp4sim::graph::Invocable *source_producer;  // via owner_ — the frontier owner
    bool unlinked = false;    // UC-14: no graph edge → stale-only
};

struct ModelStatus   // private-central
{
    std::atomic<uint64_t> committed_count = 0;
    std::atomic<uint64_t> committed_time  = 0;
    std::atomic<uint64_t> latest_area     = 0;
    std::atomic<uint64_t> generation      = 0;
};
```

The mapping `edges_[model][i] ↔ model->connections[i]` is **index-aligned**, which is what
lets `copy_model_inputs` iterate the model's own `connections` and just ask `resolve(model, i,
...)` — no re-keying.

---

## How `resolve` picks index vs time (internal, concealed)

```cpp
ResolvedRead resolve(Invocable *model, size_t i,
                     uint64_t step_start, uint64_t step_end)
{
    const Edge &e  = edges_[model][i];
    const ModelStatus &st = status_[e.source_producer];
    ResolvedRead r;

    // Latest (default) / UC-14 unlinked: newest committed area, direct index, no scan,
    // never the live head (determinism). No input_time — delay/time_offset are
    // time-domain knobs and do not apply to the latest index.
    if (e.unlinked || e.mode == ssp4sim::scheduling::detail::AccessMode::Latest)
    {
        r.valid   = st.committed_count > 0;
        r.is_area = true;
        r.area    = st.latest_area;                 // never the live head (determinism)
        r.generation = st.generation;
        return r;
    }

    // Index mode: absolute fixed slot, no time involved.
    if (e.mode == ssp4sim::scheduling::detail::AccessMode::Index)
    {
        r.valid   = e.source->ring->is_populated(e.fixed_index);
        r.is_area = true;
        r.area    = e.fixed_index;
        r.generation = st.generation;
        return r;
    }

    // Time modes (StartTime / EndTime): pick the base step handle the connection samples at.
    uint64_t base = (e.mode == ssp4sim::scheduling::detail::AccessMode::StartTime) ? step_start : step_end;

    // Not-yet-committed producer → no valid data (D2/D13) → skip copy this frame.
    if (st.committed_count == 0)
    {
        r.valid = false;
        return r;
    }

    int64_t ref = int64_t(base) + e.time_offset - int64_t(e.delay);
    if (cfg_.clamp_stale_shortfall)
        ref = std::min(ref, int64_t(st.committed_time));     // M1a: never read past frontier

    r.valid   = true;
    r.is_area = false;                                       // caller searches at this time
    r.time    = (ref < 0) ? 0 : uint64_t(ref);               // no unsigned underflow (D8)
    r.generation = st.generation;
    return r;
}
```

Note what the caller is **never exposed to**: `DataAccessMode`, `delay`, `time_offset`,
`unlinked`, `fixed_index`, the clamp, the `committed_count` gate. All of it is resolved
here and reduced to *"here is an index"* or *"here is a viable time"*.

---

## `copy_model_inputs` — now trivial, logic concealed

```cpp
// The *entire* read path. Runs once per model pre().
// The resolver conceals all access policy; this loop only interprets `is_area`.
// No input_time needed — Latest resolves to the latest committed index internally.
void copy_model_inputs(ReadTargetResolver &res, FmuModel *target, int target_area,
                       uint64_t step_start, uint64_t step_end)
{
    for (size_t i = 0; i < target->connections.size(); ++i)
    {
        ConnectionInfo &c = target->connections[i];
        ResolvedRead r = res.resolve(target, i, step_start, step_end);
        if (!r.valid) continue;                       // keep init / skip this frame

        std::byte *src;
        if (r.is_area)
            src = c.source_storage->get_item(r.area, c.source_index);              // direct
        else
        {
            size_t src_area;
            if (!c.source_storage->find_latest_valid_area(r.time, src_area)) continue;
            src = c.source_storage->get_item(src_area, c.source_index);            // frontier-bounded
        }

        std::byte *dst = c.target_storage->get_item(target_area, c.target_index);
        copy_value(dst, src, c.size, c.type);         // type-aware copy (D15)
    }
}
```

The read loop is now **~10 lines** and contains no access-policy decisions: no mode switch,
no `time_offset`, no `delay`, no unlinked check, no frontier clamp. All of that lives in the
resolver, which is exactly the "conceal the logic regarding access heavily" intent.

---

## Executor usage (unchanged from v1 — the sole writer)

```cpp
void run_jacobi(std::vector<Invocable *> models, ReadTargetResolver &res)
{
    for (uint64_t step = 0; step < steps; ++step)
    {
        for (Invocable *m : models)
        {
            m->invoke(StepData(...));
            res.mark_committed(m, m->current_time, latest_output_area(m));  // D17 ordering
        }                                                    // barrier per step
    }                                                        // consumers read next step
}
```

---

## Notes against design constraints

| Constraint | Where v2 honours it |
|---|---|
| Construct against the models only | ctor takes `std::vector<Invocable*>`; derives status/edges/owner from `output_area` + `connections` |
| One read function, index-or-time by mode | `resolve(...) → ResolvedRead{is_area, area|time}`; no policy decision in the caller |
| Read loop only copies | `copy_model_inputs` keeps only `is_area` branch + `copy_value` (D15) |
| Frontier gates targets (M1a) | clamp `ref = min(ref, committed_time)` inside `resolve` |
| Centralized single mutable truth (M1b) | one private `status_`; `mark_committed` is the only writer; `resolve` is pure |
| Staleness signature (M1c) | `generation` on `ModelStatus`/`ResolvedRead` (optional compare by executor) |
| No invented happens-before | `committed_count == 0` → `valid = false`; unlinked reads stale-only |
| Unlinked stale-only (UC-14) | `e.unlinked` → straight `latest_area`, never the live head |
| Non-nesting basis | edges resolve via flat `owner_`; no executor tree |
| Lookback bound (D8) | ref clamped ≥ 0, no unsigned underflow; `validate()` deferred as build-time concern |
| Write side stays in executors | `mark_committed` is executor-called only |
| Physical stability while copying (D17) | commit release-store after producer's `post()`/append; copy after acquire |

---

## Intentionally deferred from v1 (kept out of the interface)

Nothing about determinism is weakened; only the *surface* is simplified. Deferred (unchanged
from v1, now *behind* the interface rather than in it):

- **`AccessPlan` / plan caching** — `resolve` runs per model `pre()`. When profiling
  demands, the resolver can cache per-`(generation, step)` results; `ResolvedRead` and the
  copy loop don't change.
- **Edge roles / scopes (M2)** — staleness/freshness still expressed through `mode` +
  `delay` + frontier. Additive later, inside `resolve`.
- **Joint-frame / D16** — documented unsupported for now.
- **`validate()`** — build-time lookback/capacity checks, pending (kept as a static helper
  in the future; not needed for the initial surface).

## Relationship to v1

| Aspect | v1 (`data_access_read_target_resolver.md`) | v2 (this file) |
|---|---|---|
| Constructor | `GraphExecutor &graph` | `std::vector<Invocable*> models` |
| Read surface | `std::vector<ReadTarget> resolve(model, ...)` + `graph()` + `validate()` | `ResolvedRead resolve(model, idx, ...)` — one call |
| Result | `ReadTarget{state, source, source_index, reference_time, generation, area, LIVE_LOOKUP}` | `ResolvedRead{valid, is_area, area|time, generation}` |
| Read loop | caller branches on `state` + `area`-vs-lookup | caller branches only on `is_area` |
| Concealment | mode/delay/offset/clamp partly in caller | **all** access policy in the resolver |

v1 remains the *analysis home* for the UC-14 determinism reasoning and the constraint
mapping; v2 is the streamlined external contract intended to make integration against
`FmuModel`/`copy_model_inputs` seamless.

---

## Future revisions

The non-negotiable anchors are: centralized per-model status, single committed frontier as
the only mutable truth (M1b), frontier-clamped reads that never touch the live head
(M1a/D2/D7), unlinked reads stale-only (UC-14), and `mark_committed` as the sole writer.
Any future surface change must preserve those.

Status of the initial implementation (lib/include/scheduling/read_target_resolver.{hpp,cpp}):

- [x] Constructor against `std::vector<Invocable*>` (FmuModel cast; non-Fmu nodes skipped)
- [x] `mark_committed` (release-store) + `resolve` (index-or-time) public surface
- [x] Pluggable `detail::ReadResolver` interface; both `mark_committed` and `resolve`
      forwarded to the injected resolver
- [x] `detail::LatestExecutedResolver` + `detail::MacroStepStartTimeResolver` concrete
      strategies, split into their own file pairs; unit-tested (modes, delay/offset, clamp,
      first-commit gate, Index, unlinked) via the shared instances
- [x] **`ConnectionInfo` is policy-free** — `DataAccessMode`/`mode`/`time_offset`/`fixed_index`
      removed; `retrieve_model_inputs` retired
- [x] **Graph builder integration** — resolver built in `build_simulation_graph`, owned by
      `SimulationPipelineResult`, borrowed by every `FmuModel`; `pre()`/`direct_feedthrough()`
      drive `copy_model_inputs`; `post()` calls `mark_committed`
- [x] `detail::copy_connection` (FMU-free copy step: value + derivatives, D15 string-aware)
- [ ] Build-time `validate()` (D8 lookback / capacity checks) — follow-up
- [ ] Plan caching / edge roles / scopes (M2), joint-frame (D16) — deferred per design
- [ ] Feedthrough ⇒ StartTime / delayed ⇒ EndTime policy derivation — currently all wired
      edges resolve StartTime (matches the prior builder behaviour); richer derivation is a
      resolver decision layered on the same graph facts