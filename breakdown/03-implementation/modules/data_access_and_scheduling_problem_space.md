# Module: Data Access & Scheduling — Problem Space (solution-free)

<!-- Layer: 03-implementation -->
<!-- Status: Problem-space baseline. Intentionally contains NO candidate designs or interface decisions. -->
<!-- Sister doc: data_access_and_scheduling.md (full exploration incl. solution options) -->

## Purpose

This document states the **problem space** around how connections read producer data in
Gauss–Jacobi / Gauss–Seidel execution. It records the uses, the open difficulties, and the
open questions. It deliberately contains **no solution details** — no candidate designs, no
interface proposals, no storage-architecture choices, no code pointers.

The central problem: can producer data be read **deterministically** when execution is
**parallel** and when the schedule **mixes Jacobi and Seidel styles** (and is **long/deep**)?
Two fundamentally different ways of addressing stored data exist — **by time** and **by index** —
and the problem is to decide which semantics (or combination) can express the required read
behaviour without breaking determinism. A further pressure shapes the whole space: the read path
is **hot** (executes once per connected model per step), so the acceptable answer must be one
that can be **heavily pre-computed** at graph build — collapsed into near-zero per-step read
logic — while still honouring the schedule-informed cases.

---

## Vocabulary

| Term | Meaning |
|---|---|
| **area** | One time-versioned record of a producer's output (one storage slot's worth of data). |
| **head / live write pointer** | The most recently appended area; the newest visible data. |
| **committed frontier / watermark** | A frozen read boundary. Reads resolve relative to this, never to live writes. |
| **lookback** | How many committed steps back a connection reads. |
| **phase / epoch** | Schedule identity carried by the executor (macro-step, sub-iteration). |
| **connection** | One typed edge from a source producer to a target consumer. |
| **SCC** | Strongly Connected Component; nodes that feed back on each other and must iterate inside a step. |

---

## Problem space axes

Five independent axes; the design tension is that today they are conflated:

1. **What to sample** (value style): latest-valid ≤ t, exact t, step start, step end, a specific
   stored point, or "just-written / fresh".
2. **How to address storage**: by **time** (search) or by **index** (direct address).
3. **Who decides / when**: a **static per-connection** setting decided at graph build vs a
   **schedule-driven** runtime frontier; and *when in the schedule* the choice is made
   (macro-step vs sub-step, whether the read boundary may move mid-batch).
4. **Visibility / persist-what**: which producer state external observers (recording / export)
   see, and whether that stays consistent with what consumers are legally allowed to read.
5. **Pre-computability / hot-path cost**: how much of a connection's read decision is
   *statically decidable* at graph build (fixed for the whole run) vs *schedule-informed*
   (only known at schedule runtime). The per-read path is hot — it executes once per connected
   model per step — so the problem space includes the demand that each read cost stay near-zero,
   and that as much as possible be **computed once, up front**, with the runtime read doing
   direct memory access rather than search or dynamic decision.

Mixing Jacobi/Seidel in one schedule turns axis 3 into a *continuous per-connection* control:
different edges may intentionally be **stale** (Jacobi-like), **fresh** (Seidel-like), or
**delayed** (more than one step back). The core problem is whether any single read model can
express all three deterministically.

### Pre-computation push (the axis-5 tension)

There is a design pressure to make the hot read path **heavily pre-computed**:

- **What is statically decidable** — per edge, at graph build: the producer storage, the value
  style, the lookback rule, and — for fixed, non-cyclic graphs — even the *physical location*
  to read in a ring the size of which is known up front. Pre-computing this yields a read loop
  with essentially zero per-step logic per connection: load an already-resolved address, copy.
- **What is not statically decidable** — anything tied to schedule runtime: which frontier is
  legally visible *right now* (UC-5 SCC sub-iterations, UC-6 branch decoupling), whether a
  "fresh" edge may legally read at all (sequential ordering), first-step behaviour before any
  commit (UC-11), and roll-back to a checkpoint (UC-10).
- **The problem formulation:** *at graph build, how much of the per-connection read decision can
  be collapsed into a static plan that the runtime simply executes — and for the remainder, what
  is the smallest dynamic delta that must stay schedule-informed — such that determinism is
  preserved and the hot path never pays for the general case?* A useful subordinate question:
  whether edges can be *classified* into fully-static and schedule-informed classes, with the
  hot path fixed up front and the dynamic cases kept few, cheap, and isolated.

---

## Use-cases

| # | Use-case |
|---|---|
| UC-1 | Pure serial Jacobi: every node reads the prior step's output. |
| UC-2 | Pure parallel Jacobi: identical result regardless of thread finish order. |
| UC-3 | Pure serial Seidel: downstream reads the just-written value within a sweep. |
| UC-4 | Mixed graph: some edges fresh, some stale, some delayed (per connection). |
| UC-5 | SCC iterative convergence: iterate inside a step and advance the visible frontier each iteration. |
| UC-6 | Long/wide graph: freshness in one branch must not be gated by another branch's progress. |
| UC-7 | Delay / transport edges: delay expressed as a number of steps back. |
| UC-8 | Derivatives / close-by points: access neighbouring areas (area k, k−1, k−2). |
| UC-9 | Deterministic golden tests: reproducible output across runs. |
| UC-10 | Failed-step roll-back / restart / restore: re-run a macro step from a checkpoint. |
| UC-11 | Initialization / t=0: first-step reads with no prior commits. |
| UC-12 | Recorder / export accuracy: exported timeline matches what consumers read. |
| UC-13 | Multi-rate / non-dividing sub-steps: mixed-rate models share a step; sub-steps break the "one area = one step" identity. |
| UC-14 | **Unlinked data access**: a model reads a producer with **no explicit graph edge** (no connection, so no DAG/wavefront happens-before), and the read must still be deterministic under parallel execution. Since the schedule grants no ordering, such reads are **stale-only by construction** — they may resolve only against the committed frontier, never the live write head; "fresh" access without an edge is illegal (a graph-construction fault, not a lookup). |

---

## Known difficulties (problem-side)

### P1 — Ring wrap makes absolute indices fragile
A fixed physical slot's *contents* change step once storage wraps; the identity of what a slot
holds is lost without a generation/epoch check. Existence of data in a slot is not the same as
validity of that data for the intended step.

### P2 — Offsets from a live write pointer race under parallel producers
If a consumer reads "one step back from the newest" while its producer appends concurrently, the
result depends on whether the producer happened to have appended yet — outright
non-determinism. Corner cases multiply: a reader can collide with the exact slot being written
right now (torn read), and the very first batch can underflow before any commit exists, silently
returning stale/init data instead of what a serial run would see.

### P3 — "Newest" (lookback 0) is schedule-dependent, always
"Fresh" is only meaningful when the producer is ordered *before* the consumer in this pass. That
ordering is a property of the schedule, never of the storage itself. Under parallel execution,
fresh reads are inherently racy. But the same boundary problem extends to **stale** reads at the
first-commit moment: serial and parallel schedules may legitimately disagree on step-1 results.

### P4 — A single global frontier couples independent branches
With one shared read frontier, a slow producer in one branch throttles visibility in an
unrelated branch: the frontier can advance only as far as the *slowest* producer, and can even
expose a fresh value to a consumer whose own prerequisites aren't sound.

### P5 — Per-producer frontiers must not desync external observers
Decoupling branches requires per-producer frontiers — but every producer also has a third
observer besides the scheduler and consumers: the recorder / exporter. If recording runs on the
producer's own thread, a frontier advanced elsewhere can race the not-yet-committed data it is
copying, and a lagging recorder can fall behind a fast-moving frontier until the exported
timeline diverges from the read timeline.

### P6 — History window has a hard size and a real corner case
To read `l` steps back you need `l+1` distinct retained areas, not `l`. One-area storages are
real: a lookback ≥ 1 on them silently returns the newest data. Combined with the torn-read bound,
the usable history window is smaller than nominal capacity.

### P7 — Schedulers cannot express per-connection policy today
Schedulers and models exchange only a fixed step descriptor; the scheduler has no view of
connections, read modes, or offsets. A schedule-driven read policy needs a defined boundary for
who (executor vs model) is allowed to decide sampling policy.

### P8 — Time is the only schedule-invariant anchor
Nodes do not know when other producers start or stop (parallel). Any index-based addressing must
therefore be anchored to a stable frontier, not to a live write pointer.

### P9 — Interpretability
Mixed read semantics make profiling and logging harder to interpret; effective per-connection
read style must be observable. (Lowest-priority item vs. the correctness risks above.)

### P10 — Correctness across parallel variants
Different parallel execution variants synchronize differently, and sync guarantees differ per
mechanism (some give defined ordering, others only partially or implementation-defined).
Stale and fresh guarantees must be argued per variant, and variants that are stubs cannot carry
any guarantee.

### P11 — Sub-steps break the "areas = steps" identity
Output time selected inside a step can fall before the step end; every "lookback = k steps"
assumption silently depends on a one-push-per-step identity that sub-steps destroy (UC-13).

### P12 — Recording precedes legal reading
A push can become visible to export before consumers may legally read it — tearing both
deterministic-output and recorded-accuracy guarantees (UC-9, UC-12).

### P13 — t=0 has no defined read frontier
Before the first push there is no valid data to resolve reads against. The very first step's
behaviour is undefined unless an explicit initialization rule is stated.

### P14 — Irrevocable frontiers conflict with rollback
A frozen, irrevocable commit frontier conflicts with failed-step re-rolls, checkpoints, and
co-simulation re-negotiation, where re-running from a restored state is expected (UC-10).

### P15 — Slot re-use can corrupt complex-typed data
Re-writing a slot that holds a complex value (e.g. a string) by raw byte copy creates aliases,
double-frees, and leaks. If a scheme makes slot re-use the normal path, every re-write must be
type-aware.

### P16 — Joint freshness across different producers has no reconciliation
Per-producer frontiers give per-edge determinism but no cross-producer consistency: a consumer
that differences two sources can read one at step k and the other at step k−3 with no detect or
reconcile mechanism. "One branch doesn't throttle another" holds only if a consumer's edges are
independent stale reads — an assumption that must be stated or solved.

### P17 — The cheapest form of the hot path is a static plan, but the schedule is dynamic
The strongest pre-computation is achieved when every connection's read decision is fixed at
graph build and the runtime read is a straight-line copy from an already-resolved location.
But the cases the schedule really cares about — fresh edges, SCC sub-iterations, branch
decoupling, t=0, rollback (UC-3, UC-5, UC-6, UC-11, UC-10) — are exactly the *dynamic* ones.
The difficulty is that the static plan and the schedule-informed behaviour are in tension on
the same edges: pushing a decision to build time requires assuming a schedule shape that
parallel execution cannot guarantee, and keeping it at runtime pays the dynamic cost every
step. Nothing in the current framing states which edges may be pre-collapsed to a fixed address
and which must stay schedule-dependent.

### P18 — Pre-computation can mask semantics until they collide
A heavily pre-computed read path is cheap precisely because the decision logic is gone — the
semantics are encoded implicitly in a fixed address. Two precomputed edges that are legal in
isolation can still be mutually inconsistent at runtime (P16), and a precomputed address can
silently serve the wrong data the moment a dynamic case (rollback, sub-step, ring wrap)
invalidates the build-time assumption, with no check left in the hot path to notice. The
problem is keeping the "what should I read" knowledge explicit and auditable even when the
runtime expression of it is fully unrolled.

---

## Open questions (problem-side)

1. **Determinism contract (gates UC-9, UC-2):** is the requirement *same schedule ⇒ same result*
   (weak) or *any schedule ⇒ same result* (strong)? Evidence so far suggests strong is
   unachievable in general (first-commit boundary, any fresh edge), so the contract must be
   scoped, and golden tests must state whether they lock serial results.
2. **Rollback / checkpoint / failed-step resume** vs a frozen committed frontier — re-runnability
   is already a hard expectation of the system.
3. **Recorder visibility:** when a push is committed vs read, does the exporter flush before the
   read frontier advances, and what exactly is recorded?
4. **t=0 / initialization frontier:** what is the rule for first-step reads before any commit,
   and what does an un-committed producer resolve to?
5. **Sub-step → area mapping:** how "1 step back" maps to stored areas when sub-steps break the
   one-push-per-step identity; multi-rate stepping.
6. **Who may decide sampling policy** — the executor or the model — and via what boundary,
   without leaking per-connection details into the scheduler.
7. **Interaction of time-based sampling styles** with the above — kept as-is or replaced; includes
   delay executors and the existing Seidel–Jacobi hybrid proposal.
8. **History-window enforcement:** how the size bound (P6) is guarded — configuration,
   build-time assertion, or runtime clamp-and-warn.
9. **Cross-producer joint freshness (P16):** whether a consumer can ever depend on several
   producers being at the same frontier, and how that is expressed or forbidden.
10. **Pre-computation boundary (P17/P18):** how much of the per-connection read decision is
    collapsed into a fixed build-time plan, and how that is reconciled with the inherently
    schedule-informed cases — including whether edges are explicitly classified into
    fully-static vs schedule-informed, and how the auditable statement of what each edge
    reads survives the unrolling of the hot path.
11. **Unlinked-read contract (UC-14):** how "no explicit graph edge ⇒ stale-only by the
    committed frontier" is *enforced* — is it a property of the model's connections registry
    (a `unlinked` edge that itself is a graph fact), or does the read path need an explicit
    assertion in `validate()` (build-time fault for fresh/serial access)? And what does the
    read resolve to when the unlinked producer has *never* committed at all?