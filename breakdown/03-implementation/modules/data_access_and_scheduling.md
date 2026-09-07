# Module: Data Access & Scheduling — Index-Based Reads in Mixed Jacobi/Seidel
<!-- Layer: 03-implementation -->
<!-- Stable ID: IMP-MOD-DATAACCESS-001 -->
<!-- Status: Exploration note (not a decision) — incorporates critical reviews (concurrency, storage, doc-framing, D6-interface) -->

## Purpose

This document maps the **problem space** around how connections read producer data in
Gauss–Jacobi / Gauss–Seidel execution, whether **index-based area access** can replace or
augment today's **time-based lookup**, and how to keep reads **deterministic under parallel
execution and on long DAGs**. It records known difficulties, use-cases, candidate designs, and
the results of **four independent critical reviews** (concurrency/determinism, storage/data
layout, document framing, and the D6 interface boundary). It deliberately does **not** make a
binding decision yet — the reviews sharpened the constraints, but several open questions remain.

Sister module docs: `execution.md` (strategies), `signal.md` (storage/recording),
`loop_aware_scheduler.tldr` (SCC-oriented sketch that spawned this exploration),
[`IMP-025`](../../06-evolution/backlog/candidates/IMP-025.md) (existing Seidel–Jacobi hybrid
proposal — a related, pre-existing design that already partitions SCC vs. non-SCC).

---

## Vocabulary

| Term | Meaning |
|---|---|
| **area** | One time-versioned record inside a `SignalStorage` (one ring slot). |
| **slot** | Physical buffer position; `head = write_count % capacity`. |
| **head** | The most recently **appended** slot (the live write pointer). |
| **watermark / commit** | A scheduler-frozen read frontier. Reads resolve *relative to this*, never the live head. |
| **lookback** | How many committed steps back a connection reads. *Definitions must be made consistent:* from a committed frontier, Jacobi's stale read is **lookback 0 at the batch barrier** (see "Review corrections"); the status-quo time-scan view phrased this as `≥ 1`. |
| **phase / epoch** | Schedule-identity carried by the executor (macro-step, SCC sub-iteration). Under R5 a single scalar per invoke; connections map `(edge, phase) → style`. |
| **connection** | `ConnectionInfo`: one typed edge from a `source_storage`/`source_index` to a `target_storage`/`target_index`. |
| **input_time / start / end** | The three time handles the current `retrieve_model_inputs` can sample at. |
| **SCC** | Strongly Connected Component; nodes that feed back on each other and must iterate inside a step. |

---

## Current data-access model *(corrected)*

`FmuModel::pre()` calls `retrieve_model_inputs(connections, target_area, input_time, step_start, step_end)`.
Each connection resolves its **source area by time** via `ConnectionInfo::mode`:

| Mode | Samples at |
|---|---|
| `StartTime` | `step_start` |
| `EndTime` | `step_end` |
| `LatestTime` | `input_time` (zero-order hold) |
| `Index` | **fixed physical slot** `fixed_index`, no time lookup (added recently, absolute semantics) |

**Two corrections from review 3 — what the code *actually* does today differs from intent:**

1. **The builder pins every connection to `StartTime`** (`sim_graph_builder.cpp:326`, `time_offset = 0` at `:327`).
   No edge in the current build samples `input_time`. Therefore the `SerialSeidel` `input_time = end`
   (`seidel_serial.cpp:32-36`) currently cannot produce the "later-in-sweep node sees a freshly
   written value" behavior — every feedthrough edge reads `step_start` (i.e. the previous macro
   output). **"Seidel in-sweep feedback" is a design target, not a current fact.**
2. **`ConnectionInfo::time_offset` is a live, half-plumbed scheduler knob** — it is honored in the
   read path (`model_connection.cpp:72`: `reference += connection.time_offset`) and `graph.md:24-26`
   documents it as "an executor/scheduler algorithm can tune when a connection samples". It is
   **only ever set to 0**. It is the intended "who decides" mechanism — the D6 analysis below shows
   why it alone is insufficient, but it must not be ignored.
3. `StepData::use_input_time` / `use_output_time` are **dead flags** (declared, never read).

The **executor** only influences data access through the `StepData` it constructs:

- `JacobiSerial` builds `StepData(start, end, substep, start, end)` → `input_time = start`.
- `SerialSeidel` builds `StepData(start, end, substep, end, end)` → `input_time = end` (latent).

So the Jacobi/Seidel distinction lives in *which timestamp is requested*; but actual sampling is
pinned to `StartTime` by the builder, and every lookup is a **newest-first scan** bounded by
`min(write_count, capacity)` (`ring_buffer.cpp:143-162`).

---

## Review corrections to the performance / cost story  *(review 2)*

The original draft claimed "every lookup is an O(capacity) reverse scan". **This is wrong for the
target regime.** `find_latest_valid_index` walks newest-first and returns on the first match:

- Steady-state Jacobi: `input_time = start` = previous output head ⇒ **matches on iteration 1**.
- Steady-state serial Seidel: `input_time = end` = the head just written ⇒ **matches on iteration 1**.

The scan only deepens for genuinely **old** lookup times (delay / `time_offset` / lagged
`StartTime`/`EndTime`), depth ≈ `(head_time − lookup_time)/dt`, worst case `capacity`. Capacities are
10 (input) and 200 (output) (`model_fmu.cpp:28-29`), and `retrieve_model_inputs` runs once per
model `pre()` (`model_fmu.cpp:151`), not per connection per step.

**Conclusion: Option C's real value is determinism (D2), not speed.** The "O(capacity) → O(1)"
headline overstates the win; the genuine O(1) gains are narrow — deep-delay edges and removing the
per-iteration modulo. Before building, micro-measure actual scan depth in the target models; if it
is ~1, the speed argument for a storage re-architecture collapses and the decision becomes a pure
determinism argument.

UC-8's "adjacent areas are cheap" also needs a caveat: `slot−1 ≠ step−1` in general, because
`get_or_push` reuses exact-time areas without pushing (storage.cpp) and `set_initial_input_area`
pushes `capacity` slots at one timestamp (model_connector.cpp). "One commit == one push" is an
invariant the design must *state* (see D8/D15/D16).

---

## Problem space (the axes)

Four independent axes, and the design tension is that they are conflated today:

1. **What to sample** (value style): latest-valid ≤ t, exact t, step start, step end, or a
   specific stored point.
2. **How to address storage**: by **time** (scan) or by **slot index** (direct address).
3. **Who decides / when**: a **static per-connection** config (graph build) vs a
   **scheduler-driven** runtime watermark; and *when in the schedule* the choice is made
   (macro-step vs sub-step phase, whether the frontier is allowed to move mid-batch).
4. **Visibility / persist-what**: which producers' state the **recorder/export** sees (append vs
   commit vs callback), which must stay consistent with what consumers may read (added from review 3).

Adding "mix Jacobi/Seidel in one schedule" makes axis 3 a *continuous per-connection* control:
different edges may intentionally be **stale** (Jacobi-like), **fresh** (Seidel-like), or
**delayed** (lookback > 1). The core question is whether index addressing can express all three
deterministically.

---

## Use-cases

| # | Use-case | Access needed | Index relevance |
|---|---|---|---|
| UC-1 | Pure serial Jacobi | Every node reads prior step's output | lookback from a **stable committed boundary** |
| UC-2 | Pure parallel Jacobi | Same as UC-1 but **identical result regardless of thread finish order** | Stale access naturally survives parallelism (never races with append) |
| UC-3 | Pure serial Seidel | Downstream reads just-written value in sweep | lookback = 0/fresh, valid *only* when scheduler gives happened-before |
| UC-4 | Mixed DAG (some edges fresh, some stale) | Different lookback per connection | The raison d'être |
| UC-5 | SCC iterative convergence | Inside a step, iterate an SCC and advance the visible frontier each iteration | Scheduler "updates the index for the SCC between iterations" (tldr sketch) |
| UC-6 | Long/wide DAG | Freshness in one **branch** must not be gated by another **branch's** commits | Requires per-producer watermarks (see D4/D5) |
| UC-7 | Delay / transport edges | `delay` expressed as steps back | lookback = delay; compose with `ConnectionInfo::delay` (already a time-domain subtraction, `model_connection.cpp:74`) |
| UC-8 | Derivatives / close-by points | Value at area k and area k−1, k−2 for interpolation | `derivative_locations` adjacent; **only if one-commit-one-push holds** (see review corrections) |
| UC-9 | Deterministic golden tests | Reproducible output | Any index scheme must give schedule-independent results — *determinism contract is the gating question (Q1)* |
| UC-10 | Failed-step roll-back / restart / restore | Re-run a macro step from a checkpoint | **An irrevocable commit frontier conflicts with "models cannot be reset"** (`loop_aware_executor.cpp:264`) — added from review 3 |
| UC-11 | Initialization / t=0 | First-step reads with no prior commits | Need an init-commit rule before the first batch (added from reviews 1 & 3) |
| UC-12 | Recorder / export accuracy | Exported timeline matches what consumers read | Commit-vs-append must not desync the recorded stream (added from reviews 2 & 3) |
| UC-13 | Multi-rate / non-dividing sub-steps | Mixed-rate FMIs share a step | Sub-steps break the area↔step identity every "lookback = k" assumes (added from review 3) |

---

## Known difficulties

### D1 — Ring wrap makes absolute indices fragile
`fixed_index` (current `Index` mode) addresses a **physical slot**. Once the ring wraps
(`write_count ≥ capacity`) that slot's *contents* belong to a different step. Because
`get_item` returns a pointer into a pool allocated once, **the pointer never dangles** — the
failure is worse: the same slot silently serves a *different step's* data (identity loss), with
no generation/epoch check. The current `Index` mode's `is_populated()` guard is only a
*lifetime* test, not a *validity* test.

### D2 — Head-relative offsets race under parallel producers *(hardened by review 1)*
If a consumer reads "lookback 1 from head" while its producer appends concurrently:
```
B consumes A.out (lookback=1), parallel batch:
  A already appended step T  → B reads T−dt   (correct)
  A not yet appended step T  → B reads T−2dt  (wrong / non-deterministic)
```
Review 1 found **two additional concrete breaches beyond the phrasing above**:
- **Torn read at `lookback = capacity − 1`.** The in-batch append targets `(committed+1) % cap`;
  a reader at `(committed − lookback) % cap` collides with it exactly when `lookback ≡ −1 (mod cap)`,
  i.e. **`lookback = capacity − 1`**. That slot is being written right now (`push` sets `populated`
  *before* `read_values_from_model` writes the value bytes — `ring_buffer.cpp:56-58` vs
  `model_fmu.cpp:179-181`), so the reader observes a half-appended slot *every batch*. D8's
  window check does not exclude this → the usable bound is `lookback ≤ capacity − 2`.
- **First-commit underflow.** `committed_count` starts 0. In the first parallel batch nothing has
  committed; `(0 − 1)` in `size_t` wraps to `2⁶⁴−1`, `% cap` → slot `cap−1`, unpopulated →
  `found = false` → `retrieve_model_inputs` skips the copy (`model_connection.cpp:80-126`) → the
  consumer's input keeps its **init** value. For *output*-sourced edges (output areas are never
  pushed at init) this is a **behavioral regression of step 1** vs. the time-scan that reads the
  initial area. The doc's "clamped to ≥ 0" is vacuous on unsigned types — the subtraction wraps
  *before* the clamp. (Partially masked for input-sourced edges because `set_initial_input_area`
  pre-fills all capacity slots, `model_connector.cpp:53-80`.)

### D3 — "latest" (lookback = 0) is schedule-dependent, always
lookback = 0 only means something stable when the producer **committed before** the consumer ran
in this pass. That ordering is a property of the **schedule** (edge direction / wavefront), never
of the storage itself. Under parallel execution, lookback = 0 is a full race.
→ Freshness can only be *offered* by the scheduler, not *promised* by the storage. Review 1
confirms this but shows the same evil extends to lookback ≥ 1 at the **first-commit boundary**
(see D2): batch-1 parallel sees no data while a serial per-node-commit schedule sees the
producer's first commit → the strong determinism claim (Q1) breaks for **stale** reads too,
unless init-commit is special-cased.

### D4 — A single global watermark couples branches (over-limiting on long DAGs)
With one shared `committed_count`, a slow producer in branch E throttles visibility in an
unrelated branch C. The frontier can only advance to the *minimum* commit across all producers,
and can even expose a producer's fresh value to a consumer whose own dependencies aren't sound yet.
Note (review 1): in a **pure** parallel-Jacobi barrier that commits all producers each step,
per-producer == global, so D4 only *materialises* in mixed mode — the regime this doc targets.

### D5 — Per-producer watermarks decouple branches but need per-storage sync + a third reader
The fix for D4 is per-`SignalStorage` watermarks. Each storage's append+commit must be internally
synchronized. **Review 2 adds a critical missing reader: `DataRecorder`.** The recorder's copy is
**not** async — `enqueue_event → RecorderStorageBuffer::try_push` runs on the *producer* thread
inside `flag_new_data` (`recorder.cpp:121-143`, `record_tracker.hpp:65-79`), copying ring bytes.
So:
- a watermark advanced by a scheduler/barrier thread races the non-atomic `push` state on the same
  object → **commit belongs on `SignalStorage` (or a scheduler-owned per-producer map), not on
  `RingBuffer`**;
- the recorder's own buffer is only `capacity=50` with drop-and-warn (`recorder.cpp:44`,
  `try_push` fails). If commit "once per barrier" outruns recorder consumption, the exported
  timeline and the read timeline diverge → recording/export against a frozen frontier is an open
  design surface, not a detail (UC-12).

### D6 — The executor can't see connection modes today *(expanded — see §Interface boundary)*
`ExecutionBase`/`Invocable` only exchange `StepData`; executors hold `Invocable*` and cannot read
or modify `ConnectionInfo::mode/fixed_index`. A scheduler-driven scheme needs an interface-boundary
decision. **This now has its own analysis section below** (§"D6 interface boundary: responsibility
shuffling"), with a part-A inventory of the current surface and five ranked designs (R1–R5).

### D7 — Time is the only scheduler-invariant anchor
From the tldr sketch: *“tid är enda som är statiskt”* (time is the only static thing). Nodes don't
know when other producers start/stop (parallel), so index addressing must be anchored to a stable
committed frontier, not a live write pointer. (Review 3 note: the quote in the tldr is spelled
"tiden är enda..."/"tid är ända..." variants; minor, do not over-quote.)

### D8 — History-window sizing is off-by-one and has a corner case *(review 2)*
Rolling overwrite evicts old areas. To read lookback `l` you need `l+1` distinct areas
(the committed area plus l older) — **`lookback + 1 ≤ capacity`, not `lookback ≤ capacity`**.
And `areas=1` storages are real (`tests/lib/core/test_sqlite_recorder.cpp:293`), where lookback ≥ 1
silently returns the newest data → require `areas ≥ 2` for any index history. Combined with D2's
torn-read bound, the usable window is effectively `lookback ≤ capacity − 2`.

### D9 — Interpretability
Mixed semantics make profiling/logging harder; `ConnectionInfo::to_string()` must surface the
effective per-connection read style. (Review 3: this is the *least* important item on a list
that omits real correctness risks — rank it accordingly.)

### D10 — Correctness across all parallel variants
`jacobi_parallel_*` synchronize differently. Review 1: the commit-barrier story does **not**
transfer uniformly — `std::future::get` gives defined happens-before; the spin pool's `done`
atomic does only if reset/re-entry semantics are pinned; **TBB `std::execution::par` does not
formally synchronize the element invocations with the caller (implementation-defined)** → D10
requires tying the commit to a documented implementation contract per variant. Also (review 3):
`seidel_parallel` is a **stub that throws** ("This is not implemented",
`seidel_parallel.cpp:29`) — no watermark claim can hold for a stub.

### D11 — Sub-step / mid-step output-time determinig *(review 3, NEW)*
`invoke_sub_step` selects `output_time` = `substep_end`, or `substep_start + node->delay` when
the step is longer than the model delay (`executor_utils.hpp:16-33`), and the comment concedes a
pre-`end` output time "could be used non-deterministic". Every "lookback = k steps" silently
assumes area↔step identity; sub-steps destroy it (UC-13).

### D12 — Record / visibility coupling *(review 3, NEW)*
`push → flag_new_data → DataRecorder` path means an *uncommitted* push can be exported before
consumers may legally read it, tearing UC-9/UC-12 (see D5).

### D13 — Initialization / t=0 *(reviews 1 & 3, NEW)*
Before the first push, `find_latest_valid_area` returns nothing and `retrieve_model_inputs` warns
"No valid data" (`model_connection.cpp:122`) — the clamp "≥ 0" has an *undefined initial frontier*.
Needs an explicit init-commit rule (see D2 first-commit underflow).

### D14 — Rollback / checkpoint vs irrevocable commit *(review 3, NEW)*
A frozen, irrevocable commit frontier conflicts with failed-step re-rolls and FMI co-sim
re-negotiation; the repo already constrains "sub-steps must advance time; models cannot be reset"
(`loop_aware_executor.cpp:264`). A commit design must either be re-loadable or exclude rollback
(UC-10).

### D15 — String lifetime hazards under index re-use *(review 2, NEW)*
`allocate()` `construct_at`s a `std::string` in every slot (`storage.cpp:128-133`); FMU writers
copy-assign (`model_io.cpp:43-50, 89-94`); but `retrieve_model_inputs` copies with a bare
**`std::memcpy`** (`model_connection.cpp:93`). For a string-typed connection this bitwise-copies the
string *object*: alias + double-free at destruct (`destroy_at`, `storage.cpp:36-46`) + leak of the
prior buffer on rewrite. Commit-driven **deterministic slot re-use makes this the normal path**, not
a rare edge. Fix: type-aware copy (destroy+reconstruct or copy-assign) wherever a slot is re-written.

### D16 — Fan-in joint-freshness *(review 1, NEW)*
Per-producer watermarks give *per-edge* determinism but **no cross-producer consistency**: a consumer
that differences two sources can read C at step k and B at step k−3 with no detect/reconcile
mechanism. "One branch doesn't throttle another" (D4/D5) holds only if a consumer's edges are
independent stale reads — an implicit assumption that must be stated or solved independently.

### D17 — Memory ordering: the commit counter must be atomic *(review 1, NEW)*
The scheme works only with: `std::atomic<size_t> committed_count` as a **release-store** on commit
issued *after* value bytes are visible, and an **acquire-load** on the reader's side before the
`memcpy`. A plain scalar is a data race (UB) and, on weak-order ISAs, the byte load can be hoisted
above the counter load. No fence/atomic rules are stated anywhere in the current draft.

---

## D6 interface boundary: responsibility shuffling  *(new section, from dedicated review)*

### Part A — inventory of the current boundary
- Executor sees, through `Invocable*`: `id`, `walltime_ns`, `temporal_type`, `delay`, `current_time`,
  `realtime`, `parents/children` as `Node*`, and `invoke(StepData)` + lifecycle. **No** connections,
  modes, offsets, or storages.
- `ConnectionInfo` lives in `FmuModel::connections` (incoming edges), filled by
  `GraphBuilder::wire_connections` (`sim_graph_builder.cpp:351`) — each model owns its consumer side.
  Reached from an executor today **only** via dynamic_cast at build time (`sim_graph_builder.cpp:28`),
  plus a pre-existing C-cast hack in `custom_executors.hpp:107` (`(FmuModel*)node`) — a wart to
  remove regardless of chosen design.
- `time_offset` is honored but pinned to 0 (= A.4 in review terms); `use_input_time/output_time` are dead.
- The only runtime dial today is `StepData.input_time`.

### Part B — verdict on the three D6 options
- **D6a (plumb a handle)**: workable, thread-safe (single-writer between batches), but worse
  cleanliness — strategies learn `ConnectionInfo` semantics and would have to classify SCC topology
  themselves for UC-5.
- **D6b (whole schedule in StepData)**: wrong at full granularity (O(edges) copied by value into
  every parallel task). The good version is the O(1) scalar (R5 below).
- **Resurrect `time_offset`**: answers "which knob" but not "who turns it"; and a time-domain
  shift reaching the live head re-opens D2. It is the right *mechanical* dial for shifting a
  sampling instant, but can't encode the committed-frontier advance (D1/D7).

### Part C — candidate responsibility-shuffling designs (ranked)
| Id | Idea | Concrete surface | Dep direction | Thread-safety | Fit with strategy pattern |
|---|---|---|---|---|---|---|
| **R5** | Executor emits a **phase/epoch scalar on StepData**; models own static per-edge rule `f(edge, phase)` | `StepData.phase/scc_iteration` (O(1), by-value) + `FmuModel::access_plan` built at graph build; `pre()`/`retrieve_model_inputs` take `phase` | executor→StepData only; model interprets | inherited from by-value StepData | highest (factories stay policy-free) |
| **R3** | Split `ConnectionInfo`: static topology + shared dynamic `AccessPolicy` objects | New `scheduling/access_policy.hpp`; shared `shared_ptr` held by both `FmuModel` and scheduler | executor→AccessPolicy only | single-writer/barrier, no hidden global | high |
| R2 | Executor publishes a `PhaseBoard` token; connections self-map | `atomic<PhaseToken>` board in execution/ | model reads board | atomic, but hidden shared state + nesting wart (LoopAware owns sub-executor) | medium |
| R1 | Scheduler-facing virtuals on `Invocable` (`apply_access(patch)`, edge-count) | new virtuals + tiny `access_policy.hpp` | executor→Invocable + AccessPolicy | single-writer/barrier | low (base-interface leak) |
| R4 | Read-only `PhaseView` snapshot DTO | `Invocable::connection_snapshot()` virtual | report-only | n/a (can't mutate alone) | must pair with R1-3 |

Ranking: **R5 = R3 (top)** for cleanliness; R5 wins on minimal surface and inherited
concurrency safety. R4 is the observability complement, not a stand-alone.

### Part D — critique & recommendation
- R5's honest cost: every future strategy must set `phase` (a contract to honor), and a scalar
  can't express *dynamic, order-dependent* freshness per edge (needs a per-edge signal, which R5
  forbids). It solves **D6** ("who can set sampling policy") but **NOT** D5 — the commit/watermark
  *write* still needs a writer with the same shape: **`Invocable::commit_outputs()`** (no-op
  default; implicit = serial auto-commit in `post()`; explicit at parallel barriers;
  `loop_aware_executor.cpp:324-330`). Keep `time_offset` as the time-domain dial phase rules
  produce; remove dead `use_input_time/output_time`.
- **Recommended separation (the key defense):** *The executor should never interpret per-connection
  policy — it only drives a phase; connections interpret it via static, graph-built rules.*
  Concretely for UC-5: LoopAware bumps `phase` per SCC sub-iteration; the per-edge lookback rule
  lives in `FmuModel::access_plan`, not in the executor.

---

## Candidate designs *(corrected per reviews)*

| Option | What the index means | Determinism | Parallel-safe | Long-DAG | Cost (hardened) |
|---|---|---|---|---|---|
| **A — Absolute physical index** | fixed slot | Yes per slot, content shifts on wrap | Yes | No | Trivial |
| **B — Head-relative lookback** | `head − lookback` | No — sees mid-appended state (D2) | **No** | Partial | Small |
| **C — Committed watermark frontier** | `committed_count − lookback`, per producer | Yes *if* atomic + ordering (D17) + init-commit (D2/D13) | **Yes** (race removed, exceptions D2 cap−1 & D16) | **Yes** (per-producer, but D16 joint-freshness caveat) | **Storage `commit()` split + per-variant commit sites + recorder ABI implications (reviews 1&2 rate this higher than original "Small")** |
| **D — StepData-carried reference counter** | slot resolved from a ref in `StepData` | Yes if scheduler sets it identically | Yes | Partial | More `StepData` plumbing (C's watermark delivered in-band, smaller audit surface per review 3) |
| **E — Time-emulated (status quo)** | time scan | Yes | Yes for stale reads (Jacobi); **lookback-0 fresh is a race (D3)** | Yes (serial/barrier) | **O(1) in steady state, not O(capacity)** (review 2) |

**Review 3's flags:** the table compares incomparable units (implementation effort vs runtime
complexity); C's "Determinism Yes" silently inherits D3 (fresh lookback-0 still needs scheduler
happened-before) — qualified same as D; E's "Parallel-safe Yes" is only Jacobi-stale; the
"converged on C" claim (below) contradicts the Purpose. Corrected cells above; the verdict below
is now a **tentative lean**, not a decision.

---

## Where the commit boundary should live *(hardened)*

- **Per producer** (per `SignalStorage`), not global; **on `SignalStorage` (or a scheduler-owned
  per-producer map), not on `RingBuffer`** (recorder third-reader race, D5/D12).
- **Append vs commit split**: `push` advances the live head; a new `commit()` freezes the boundary.
  **Ordering contract (D17):** value bytes fully written → `commit()` as **release-store**;
  readers do an acquire-load then `memcpy`. Never call commit from `push()`. (This is a new,
  unstated invariant of the same class as the timestamp-before-head comment in
  `ring_buffer.cpp:70-78`.)
- **First-commit rule (D13):** commit the init areas *before* the first batch, and define what a
  not-yet-committed producer resolves to (not-found vs slot). Without it, serial-vs-parallel
  step-1 results diverge (D2/D3).
- **Arithmetic:** signed or clamp-before-modulo on `committed_count − lookback`; handle the
  `size_t` underflow (`index_back_from_head`, `ring_buffer.cpp:168-171`, and the watermark
  formula). Pre-validate `lookback ≤ capacity − 2` (D2 torn-read bound) and `areas ≥ 2` (D8).
- **Who drives commit** (scheduling policy): serial Seidel wavefront → per node after invoke;
  parallel Jacobi batch → once at the barrier; mixed → per-scope. Each variant's barrier must
  give defined happens-before (D10: TBB needs a documented contract).

**Net:** `lookback` (static, per-edge) + scheduler-driven `commit()` watermark (dynamic,
per-storage) = the allowed index window the scheduler continuously opens as the DAG executes —
the original framing, now with the correctness invariants it was missing.

---

## Open questions *(re-ranked per reviews)*

1. **Determinism contract (gates UC-9, UC-2, and the parallel experiment):** *same schedule ⇒ same
   result* (weak) vs *any schedule ⇒ same result* (strong). Reviews show strong is **false** for
   stale reads at the first-commit boundary and for any fresh edge — so strong must be scoped to
   "parallel-with-barrier-commit and lookback ≥ 1 (hence-after-init)", and UC-9's comparability
   axis must state whether golden tests lock serial results.
1b. (former Q1, demoted) Signed lookback (negative = look-ahead)? Lower value than the above; keep
    open only if EndTime-style predictive reads recur.
2. **Rollback / checkpoint / failed-step resume** vs irrevocable commit (D14, UC-10) — the most
   consequential omission; the repo already hard-constrains re-settability.
3. **Recorder visibility** under push/commit split (D12, UC-12): does export flush before the
   watermark advances? What commits to the recorder ring? (aligned with recorder cap=50.)
4. **t=0 / initialization frontier** rule (D13) — who commits init areas and what does a
   not-yet-committed producer resolve to?
5. **Sub-step → area mapping**: how "1 step back" maps to areas when sub-steps break
   one-push-one-step (D11, UC-13); multi-rate.
6. **`time_offset` reuse vs new plumbing** — required shape under R5; and dead
   `use_input_time/output_time` removal.
7. Interaction with `StartTime`/`EndTime` — deprecated or kept as sugar? Includes the
   `DelayExecutor` family and `IMP-025` hybrid.
8. D6's R5 vs R3: is the scalar-phase-in-`StepData` (R5) or the shared `AccessPolicy` objects
   (R3) the right boundary — decided by the D6 experiment.
9. History-window enforcement (D8): config, build-time assertion, or runtime clamp+warn, given
   the usable `cap − 2` bound and the `areas ≥ 2` check.
10. Who commits under the shared board (R2/R3): the coordinating thread only? Guarded?

---

## Experiment agenda (next steps)

1. **Serial copies**: duplicate `jacobi_serial` / `seidel_serial` as experimental drivers that
   exercise lookback semantics against a per-storage watermark, before touching parallel paths.
2. **Parallel determinism**: N producers + one consumer (lookback 1), run repeatedly under
   different thread count/scheduling; assert identical output (validates the commit-boundary race
   fix, incl. the `cap−1` torn-read bound and the first-commit rule).
3. **Long-DAG decoupling**: `A→B→C` fast chain + slow branch `D→E`, cross edge `A→D`; verify
   commits in one branch do **not** over-limit the other (per-producer watermark).
4. **Recorder visibility** (review 3, NEW): with a lagging recorder, assert the exported timeline
   stays consistent with the committed frontier under push/commit split.
5. **Micro-bench scan depth** (review 2, NEW): measure actual `find_latest_valid_index` depth in
   the target models before investing in storage re-architecture — if ~1, the win is pure determinism.

---

## Traceability

- **Backward**: `execution.md` (strategies), `signal.md` (RingBuffer/SignalStorage),
  `data-flow.md` (Flow 3), `loop_aware_scheduler.tldr` (SCC/index sketch), `IMP-025`
  (Seidel–Jacobi hybrid backlog), `graph.md` (`ConnectionInfo::time_offset` knob), quality
  attributes (parallelism, determinism).
- **Sources**: `lib/include/pre/3_simulation/elements/model_connection.{hpp,cpp}`,
  `lib/include/simulation/signal/storage.{hpp,cpp}`,
  `lib/include/utils/primitives/ring_buffer.{hpp,cpp}`,
  `lib/include/simulation/graph_executor/execution/{jacobi,seidel,loop_aware}/*`,
  `lib/include/simulation/graph_executor/execution/executor.cpp`, `executor_builder.{hpp,cpp}`,
  `lib/include/pre/3_simulation/sim_graph_builder.cpp`,
  `lib/include/pre/3_simulation/elements/model_fmu.{hpp,cpp}`,
  `lib/include/simulation/signal/recorder.cpp`, `record_tracker.hpp`.
- **Reviews incorporated**: concurrency/determinism (race walk-throughs, torn-read at cap−1,
  first-commit underflow, atomicity D17), storage/layout (perf-claim, sticky is_populated, string
  lifetime D15, index_back_from_head underflow, recorder third-reader D5), doc-framing (current-behavior
  corrections: builder pins StartTime, time_offset live, seidel_parallel stub, missing axes/use-cases/
  UC 10-13, reframe to exploration), D6-interface (Part A inventory + R1–R5 designs + recommended
  "executor drives phase, connections interpret" separation + `Invocable::commit_outputs()`).
- **Backlog ties**: see `README.md` in `breakdown/06-evolution/backlog/candidates/` for the
  `IMP-025` hybrid and related executor improvements.