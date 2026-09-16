# Module: Data Access & Scheduling — Design Space (three orthogonal designs)

<!-- Layer: 03-implementation -->
<!-- Status: Design-space analysis. Solution detail, NOT a decision. Companion to the problem-space baseline. -->
<!-- Sister docs:
   data_access_and_scheduling_problem_space.md  (problem-only, solution-free)
   data_access_and_scheduling.md                (full exploration, current-behavior corrections)
   data_access_policy_architecture.md           (candidate I — architectural, split out)  -->

## Purpose

This document lays out three orthogonal candidate designs in the already-mapped problem
space, critiques them honestly against the current system (explicitly, without polishing
the current one's weaknesses), and cross-critiques them against each other. It records the
reasoning, not a binding decision. Where a recommendation is offered, it is a first-class
lean with its caveats stated.

Designs:

- **A — Push / publish-at-commit** (dataflow)
- **B — Compile-time slot table** (static index precomputation)
- **C — Query-refined committed watermark** (the shape of the current system, fixed)

---

## The current system (stated without polish)

- Read side performs a **scan + mode switch + get-item verify per edge** on every model `pre()`.
- It walks the per-edge interpretation/scan/verification logic **at runtime**, every step.
- It carries races and management errors the reviews found: D2 (torn read at the cap−1 bound,
  first-commit underflow), D15 (bare-memcpy on re-used slots double-frees strings), D16
  (no cross-producer joint-freshness reconcile), D17 (no stated memory-ordering rule).
- These are **real defects**, not hypotheticals; the analysis below does not pretend otherwise.

---

## Design A — Push / publish-at-commit (dataflow)

The producer copies its own newly-committed values directly into each consumer's target slot
at commit time. Consumers read their own pre-filled slot.

### Strengths vs current
- Kills the per-edge interpretation/scan/verification entirely on the read side — removes the
  runtime integrity logic the current system leans on.
- Copy runs on the **producer** thread, whose buffer the producer owns exclusively — no
  cross-thread memcpy into a staging area; better cache locality.
- Fan-out structure is decided once at build; read path degenerates to a **load + one acquire**.

### Weaknesses vs current
- Lag/offset/lookback reads (Seidel→stale, delay edges) still need history, so the "re-read an
  earlier commit" case is a **second path back into storage** — you still need the ring and the
  push. Double bookkeeping.
- The producer must know **every consumer's layout** — a knowledge inversion the graph provides
  but the FMU boundary currently hides.
- t=0 / init frontier (D13) is still a first-read problem; pushing does not remove it.
- On busy-but-few consumers, the push is **wasted work** the current "copy on request" avoids.
- Sub-step mid-window commits (D11) make it ambiguous when the push is legal.
- Coupled schemas: a client FMU wants a staging-agnostic value buffer; this design forces one.

---

## Design B — Compile-time slot table (static index precomputation)

At build time, given a **fully static schedule + fixed cadence**, resolve every slot
algebraically: `slot(w) = w mod capacity`. Each edge's source resolves to a constant address
base + offset, so the runtime body is a straight memcpy stream with no loop resolution at all.

### Strengths vs current
- Absolute floor: no branches, no scan, no watermark arithmetic, no verification — the
  "precompute the access index before simulation" idea taken to completion.
- Violations are caught **at build** (cadence/capacity mismatch is a compile error, not a
  runtime surprise).
- The plan is trivially vectorizable; arrays-of-scalars phase tables.

### Weaknesses vs current — decisive
- Only works for **unconditional schedules**. `loop_aware` adapts by convergence, Seidel
  terminates by observed error, plus sub-step re-entry, rollback/recompute, and
  `find_next_valid` traversal — any run-dependent decision makes "step s ⇒ producer ordinal w"
  false after the first adaptation. The precomputed table is **stale mid-run**.
- Still needs populated/committed presence checks at read time — storage-history ≠ published
  frontier (D2/D17). It saves the address arithmetic but **not the validity check**.
- It optimizes the **cheapest part**: one acquire + one integer op per node is already nearly
  negligible. It buys microseconds while locking the system out of adaptive execution.

---

## Design C — Query-refined committed watermark (the shape of the current, fixed)

Keep the `retrieve_model_inputs`-style per-read path, but:
- per-storage **O(1) direct index** (head-derived, no scan),
- read through a static `AccessWindow.phase` rule (graph-built),
- the **committed watermark as the only truth**,
- **type-aware copies** (fix D15),
- verification **off in release**.

### Strengths vs current
- Fixes the actual defects the reviews found: D2 (torn/underflow), D15 (string double-free),
  D16 (per-edge vs joint freshness), D17 (release/acquire ordering).
- Works under **any schedule** — adaptive, sub-stepped, rollback — because resolution stays lazy.
- Migration is the smallest of the three, by a large margin.

### Weaknesses (explicit, not sugar-coated)
- Still walks per-edge; never as fast as A or B in steady state.
- Still carries a live scan for out-of-order/export reads.
- Reintroduces per-read latency if the strongest determinism is wanted without a plan.

---

## Cross-critique (honest, against the current)

| Design | Read-path cost | Schedule tolerance | Copy locality |
|---|---|---|---|
| Current | scan + switch + get-item verify per edge | any (but races/mgmt errors, D2/D17) | read-side, fine |
| A — push | ~zero (pre-filled) | long-history lags keep the ring; init frontier | producer-side, best |
| B — precomputed | true flat stream | fixed-cadence only | best |
| C — pull-refined | O(1) fixed index | any | fine |

**The uncomfortable truth the reviews converge on: none of them escape the committed-watermark
+ memory-ordering requirement (D17).** Memory visibility is a **runtime fact**; it cannot be
precomputed or pushed away. Wherever history must exist (lag, Seidel lookback, export, D11
sub-step), **A doubles the storage and B collapses**; only the pull model (current-shaped / C)
still compounds cleanly.

---

## First-class recommendation (a lean, with caveats)

For the "hot loop while keeping adaptivity":

> **A for edges without history** + **C (refined current) as the fallback** for
> lagged/history/adaptive parts, with a **B-optimized fast path** for known-unconditional
> subgraphs.

That is the honest answer — **not** "the current design is fine."

---

## Beyond A/B/C — further candidates (design space only, no lean)

The three designs above cover the **read-path × storage** corner well, but they share blind
spots that deserve separate candidates. This section records those candidates and the
reasoning around them. **No recommendation or decision is made for any of D–H here** — the
first-class lean above applies to A/B/C as originally framed, and its standing against the
candidates below is an open question. Candidate **I** (architectural: the pluggable
access-policy strategy) is documented separately in
[`data_access_policy_architecture.md`](data_access_policy_architecture.md) and linked below
— it is pointed to here so the inventory stays complete, but its reasoning lives in the
architecture doc.

### What all three designs share (and therefore miss)

- **All three take the scheduler as fixed and push determinism into storage.** The seat is
  empty for schedule-control answers: if the schedule itself guarantees happens-before for
  every read, there is nothing left for storage to defend. That is a design family none of
  A/B/C belongs to (D below).
- **All three use one scalar commit-watermark → none answers D16** (joint freshness across
  producers). The problem-space doc marks it unsolved; A/B/C inherit it equally (F below).
- **All three mutate an in-place ring → they all inherit D1/D2/D15 and patch it.** A storage
  that never reuses a slot removes that entire failure class at the source instead of guarding
  it (E below).
- **All three copy bytes per edge per step even when values do not change** (zero-order hold
  is the common case). Copy-suppression is orthogonal to all three and cheap (H below).
- **B's fatal flaw is largely fixable.** Its generalization — a finite set of precomputed
  plans keyed by schedule phase — is a genuine middle ground (G below).

### D — Schedule-discipline family (the empty seat)

The scheduler enforces a discipline that makes determinism fall out instead of defending it
in storage:

- *Level-synchronous execution*: all producers of level k run before any consumer of level
  k+1; reads are correct by construction.
- *Generation double-buffering*: two physical generations; reads always target the non-active
  one; commit = flip the generation pointer. O(1) commit, torn reads impossible by
  construction, near-zero state.

Critique: this is **Jacobi-shape enforcement**. It dies exactly where UC-3 (in-sweep fresh
reads) and UC-5 (adaptive SCC iteration) live, and it couples branches (UC-6). It is a
real commitment on schedule shape, not a free lunch.

### E — Immutable append-only commit log

Commit = publish a pointer to a never-again-written area; readers alias, never race.

- Kills D1 (no wrap → no identity loss), D2 (nothing is ever written where an old read
  points), D15 (no slot re-use → the corrupting raw-copy disappears).
- UC-10 rollback becomes pointer re-wind; UC-12 export becomes a snapshot reference.
- Costs: unbounded growth needs a reclaim/ref-count policy (reintroduces liveness questions
  of its own), plus write-alloc per step.
- Notably, it dissolves the "A doubles the storage" objection: one log serves fresh *and*
  lagged reads, so the A/C seam disappears.

### F — Per-consumer version vector

One scalar per storage becomes a per-consumer-group vector checked across all incoming edges
in a single compare. This is the only candidate here that **answers D16 instead of assuming
it away**. Cost: O(fan-in) compare per read; overkill unless a consumer actually differences
multiple producers.

### G — Phase-keyed plan template set (adaptive B)

Precompute a *set* of slot tables — steady-state table, SCC-iterate table, rollback table —
and let a phase scalar select among them. Schedule adaptation becomes a pointer flip, not a
per-read search.

- Gets most of B's flatness while surviving the adaptive families B dies on.
- Costs: templates are hand-built per executor family, and the commit/populated presence
  check survives — that part of D2/D17 is genuinely unavoidable, not an artifact of addressing.

### H — Token-skip zero-order-hold copy suppression

A monotonic version number per area; skip the memcpy when nothing changed. Orthogonal on top
of any design; matters because most samples hold between steps.

- This should be implemented, almost no matter what

### I — Pluggable access-policy strategy (execution / access graph split) — *see architecture doc*

This candidate is **architectural in kind**, not a mechanism: it answers *where the
sampling-policy decision lives and how policies are selected/composed*, and it adds no
speed or safety of its own — every aspect of its behaviour delegates to a mechanism from
**A–H**. It is therefore developed in a separate document that keeps the
architectural-vs-mechanism distinction explicit and frames the dependency surface (which
mechanism each aspect relies on) alongside it:

> **See [`data_access_policy_architecture.md`](data_access_policy_architecture.md)**
> — *Pluggable Access-Policy Architecture (candidate)*: the idea, the
> architectural-vs-mechanism distinction, what it would unify (A–H as policy instances),
> what it would buy, weaknesses, dependency surface, and open questions.

Included here in one line so the candidate inventory stays complete: a connection no longer
hardcodes a read mode; it asks an **access policy** — "resolve edge *e* at phase *p*" — and
the policy returns the index or time to read, choosing its own cost/expressiveness tradeoff
per algorithm. The mechanics of any read it returns are still supplied by A–H; the full
argument is in the architecture doc.

### Low plausibility, noted only

- *Page-alias zero-copy ownership handover*: alias lifetimes vs in-place rewrite is a
  minefield.
- *Verified-by-construction race-free schedulers*: the adaptive heuristics (convergence,
  `find_next_valid`) will not submit to the required proof.

### The sharpest open question against the existing lean

The A/C boundary ("no-history edges push, others pull") has a hidden seam: **"no history" is
not static.** The moment a graph re-parametrizes, a sub-step re-enters, or an SCC re-iterates
(D11, UC-5, UC-10), yesterday's no-history edge can become a history edge. Whether the
no-history class is closed under adaptation — and what happens when it is not — is precisely
the question E and G probe from different angles. It remains open.

---

## Traceability

- **Architectural candidate (I)**: see `data_access_policy_architecture.md` — the
  pluggable access-policy strategy (execution/access graph split), split out here so the
  design doc stays mechanism-only.
- **Problem-side**: see `data_access_and_scheduling_problem_space.md` (axes, use-cases,
  difficulties P1–P18, open questions).
- **Current-behavior corrections**: see `data_access_and_scheduling.md` (D series, review
  corpus, D6 interface analysis).
- **Known gaps carried forward**: D1–D17 in the exploration doc; D2/D11/D13/D15/D16/D17 are
  the correctness-critical ones this design space must each answer. Note which candidates
  would need to be dismissed before the existing A/C lean can stand (none are dismissed here).