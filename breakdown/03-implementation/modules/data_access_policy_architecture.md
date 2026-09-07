# Module: Data Access & Scheduling — Pluggable Access-Policy Architecture (candidate)

<!-- Layer: 03-implementation -->
<!-- Status: Architectural candidate. NOT a decision.
     Split out from data_access_and_scheduling_design.md so the design doc
     stays mechanism-only (A–H) and this architectural option has its own framing. -->
<!-- Sister docs:
   data_access_and_scheduling_design.md         (mechanisms A–H: read-path × storage designs)
   data_access_and_scheduling_problem_space.md  (problem-only, solution-free)
   data_access_and_scheduling.md                (full exploration incl. D6 interface analysis)
   data_access_read_target_resolver.md          (concrete interface mockup — split out) -->

## Purpose

This document develops one candidate in the data-access design space as an
**architectural** option: making data access a pluggable strategy chosen beside the
executor, formally splitting the **execution graph** from the **access graph**. It is
deliberately kept separate from the mechanism designs (A–H) because it differs from them in
*kind* — it answers *where the decision lives and how policies are selected/composed*, not
*how any read is performed*. Keeping the separation makes that difference explicit and
prevents the architectural question from being judged with mechanism criteria.

As with all candidates in this space, this is **a candidate — not a decision.** No
recommendation for or against it is made here.

---

## The idea

The executor is already a pluggable strategy (Jacobi, Seidel, loop_aware). This candidate
makes **data access the same kind of pluggable strategy**, chosen beside the executor
rather than fixed inside it. Formally: split the **execution graph** (who runs when;
wavefronts, iterations, barriers) from the **access graph** (per connection: which source,
how addressed, freshness/staleness/delay). A connection no longer hardcodes a read mode; it
asks an **access policy** — "resolve edge e at phase p" — and the policy returns the index
or time to read, choosing its own cost/expressiveness tradeoff per algorithm.

---

## Why this is an architectural decision, not a mechanism

The mechanism designs (A–H in the design doc) are **concrete ways to address, copy, or
order data**, each with a narrow contract: push at commit, compile-time slot table,
committed watermark, schedule discipline, append-only log, version vectors, phase-keyed
templates, copy suppression. Each can be specified, argued, and measured on its own.

This candidate is different in *kind*. It answers:

- **Where does the sampling-policy decision live?** (executor, model, or a third strategy object)
- **How are policies selected and composed?** (per edge, per subgraph, per executor-family)
- **How does policy knowledge flow?** (schedule facts out of the executor; resolved plans back in)

It does **not** by itself say how any read is actually performed. A policy strategy with no
mechanisms underneath would be an empty shell: every aspect of its behaviour still has to be
implemented by one of the mechanism designs, and each mechanism still carries the
correctness obligations (D17, D16, per-variant sync) it had before. The architecture can
therefore *only be assessed through the mechanisms it would select between* — its value is
the sum of the A–H properties it is allowed to compose, plus the composition layer, the
bind-time checking, and the freedom to tailor per algorithm. It adds no speed or safety of
its own.

Keep that in mind while reading the strengths below: they are **enabling** arguments
(architecture lets you use the fast mechanism where it fits), not **mechanistic** arguments
(architecture has no speed or safety of its own).

---

## What it would unify — the candidate designs as policy instances

Each mechanism becomes expressible as a *policy instance*, and the architecture is the
selection/composition layer over them:

| Mechanism (design doc) | As a policy instance |
|---|---|
| **A** push / publish-at-commit | eager-push policy |
| **B** compile-time slot table | static-plan (fixed-cadence) policy |
| **C** committed watermark | committed-watermark policy |
| **D** schedule discipline | generation-flip policy |
| **E** append-only commit log | immutable-log policy |
| **G** phase-keyed templates | phase-keyed template policy |
| **H** copy suppression | a layer any policy can add |

Selecting a policy per edge (or per subgraph) is then the A-vs-C-style seam, moved to bind
time. Conversely, if any mechanism cannot be expressed as a policy instance, that is a
statement about the set's heterogeneity, not an argument against the architecture.

---

## What it would buy

- **Independent replaceability.** Executor and access semantics can be swapped
  independently: keep the execution topology and change the access story per algorithm
  (Jacobi → cheap-stale, Seidel → fresh, loop_aware → phase-aware) without touching the
  executor or the connection structure.
- **Access-only graph rewrites.** A graph can be rewritten *only in access terms* — insert
  delay buffers, replace a direct edge with a copy, downgrade an edge from fresh to stale —
  while execution stays untouched. This is exactly the lever for tailoring the access cost
  to the algorithm's needs (problem-space axis 3 combined with axis 5).
- **Per-policy verification.** Each policy declares its guarantees (determinism scope,
  history requirement, compatible executor families), checked at bind against the actual
  executor — fail-fast at build, not at runtime.
- **A clean pre-computation story.** A plan-build / plan-execute split inside the policy:
  policy logic runs once per phase/epoch change; the hot read is straight execution of the
  resolved plan (problem-space axis 5).

---

## Weaknesses / costs (no gloss)

- **Indirection is only free if the build/execute split holds.** The current system's
  runtime scan *is* a per-read policy consultation. If the policy is asked per read instead
  of per epoch, this design is the status quo under a new name. The value rides entirely on:
  policy resolves once per phase change; the hot loop executes a plan whose syntactic shape
  is identical for every policy, so dispatch is bound early, not per read.
- **Two sources of truth.** Progress knowledge lives in the executor (drives it) *and* in
  the policy (reads it). If they drift, the D2/D17-class defects reappear, distributed
  across two objects. Needs a hard rule: the executor is the only writer of schedule facts;
  the policy is a pure function of them.
- **Compatibility product space.** Executor × policy is a product; most pairs are invalid
  (fresh-read policy needs happens-before; static-plan policy needs an unconditional
  schedule; generation-flip needs level-sync shape). Either every executor declares which
  policy families it can host, or bind-time checks enforce it — otherwise this *multiplies*
  the D10 per-variant correctness problem instead of reducing it.
- **Nesting.** LoopAware already hosts a sub-executor; recursive schedules need policies to
  nest too, or the inner executor silently inherits the wrong policy.
- **Storage-ownership boundary.** If the policy only chooses *addressing*, A-push cannot be
  a policy instance — push is a *write-side* schedule, which straddles execution. The
  answer decides A's fidelity inside this design.

---

## Relation to the shared blind spots

This candidate is the strongest form of "who decides / when" (problem-space axis 3) and
"pre-computability" (axis 5), but it does not outrun the underlying facts: whatever policy
runs, the committed watermark + memory ordering (D17) and the joint-freshness problem (D16)
must still be answered underneath. A policy chooses the *price* of these, not their
existence. The architecture is only the envelope; its standing depends entirely on the
mechanisms it composes, none of which it replaces.

---

## Dependency surface — which mechanisms each aspect relies on

Every aspect of the architecture's behaviour delegates to a mechanism from the design doc;
the architecture is only the selection/composition on top. Concretely:

| Aspect | What must be provided underneath | Mechanism candidates |
|---|---|---|
| Fresh / stale / delayed read resolution | A way to determine the current read target for an edge | **C** committed watermark · **A** push broadcast · **E** append-only alias |
| Reaching history / earlier commits | Retained old areas that stay readable | **E** append-only log · **C**/current ring with scan |
| Hot-path flatness (the "plan-execute" claim) | A pre-resolved plan run without per-read logic | **B** static slot table · **G** phase-keyed templates |
| Surviving schedule adaptation | A way to know which plan is valid at this instant | **G** template selection via the phase scalar · **D** schedule discipline |
| Determinism under divergence / races | A happens-before story per read | **D** level-sync or generation flip · else **C** + atomic commit ordering (D17) |
| Joint freshness across producers | A cross-*storage* consistency check | **F** per-consumer version vector · or an explicit "not supported" |
| Cheap steady state | Suppression of unneeded copies | **H** token-skip ZOH suppression |
| Rollback / re-run / checkpointing | A re-loadable frontier | **E** pointer re-wind · **C**/D re-loadable frontier · else UC-10 unsupported |

Reading across this table: **the architecture has no aspect that does not end in one of A–H.**
Selecting policy *families* per edge is therefore not a conflict with the mechanisms — it is
the statement of *which* mechanism is authorized where. Two consequences follow. First, the
architecture's cost/performance is inherited verbatim from the mechanisms it binds; it cannot
be faster or safer than the ones it allows. Second, any mechanism it forbids is simply
unreachable in that configuration — so the "which mechanisms are allowed where" decision
*is* the A–H tradeoff table, re-expressed as a capability policy. This is deliberately
stated as structure (what must exist), not as a choice (what to use): the selection itself
remains an open question.

---

## Mitigations — two sources of truth

**Anatomy.** There are *three* truth carriers, not two: executor intent (phase), storage
state (committed frontier), and policy view (what a read resolves to). Drift comes in two
kinds:
- *Semantic*: policy and executor disagree on what a phase means (a contract violation).
- *Timing*: the policy assumes "phase *p*'s commits exist" but storage has not committed
  yet, or has been rolled back — D2/D17 resurfacing at an object boundary.

The stated rule ("executor is sole writer; policy is a pure function of schedule facts")
addresses the *authority* version of drift, not the *timing* version.

- **M1a — phase selects rules, frontier gates targets.** Strip the phase of resolution
  authority. The phase only picks *which static rule/template* (G) an edge uses; the actual
  read target is always clamped to the committed frontier:

  ```
  target = clamp(apply(edge, phase-rule), committed_frontier)
  ```

  Timing drift then degrades safely: it becomes a *stale* read (correct if the rule
  permitted staleness) or a *shortfall* for fresh edges — never a torn or wrong-target
  read. Silent corruption is replaced by correct-but-conservative behaviour, which is
  observable and testable.
- **M1b — one mutable truth, one channel; policy is a method, not a state.** The policy
  holds zero mutable state: it is a pure function of (static graph, frontier, phase-label,
  scope tags). The frontier is the *only* mutable resolution input, and it changes through
  one channel (executor-issued commits). This kills the *state* version of drift by
  construction; only *timing* drift survives, which M1a handles.
- **M1c — stale-plan signature.** When a plan/phase advances, capture the frontier it
  assumed. On phase change, verify assumed == actual; mismatch (rollback, re-entry, missed
  commit) → rebuild or fail-fast. This is essentially free: it is the D17 acquire/compare
  the read already needs, reused to *detect staleness* instead of only to order the
  memcpy.

**Residual honesty.** Clamping protects reads from a *stale* policy, but not from a
*buggy* rule at phase p — an ordinary computational bug stays a bug. And the
"conservative shortfall" for fresh edges is only catchable if observability is present;
the pure-function shape makes that cheap and replayable (a test/audit projection of what
each edge would read), but it must be built.

---

## Mitigations — nesting of algorithms

**Anatomy.** Nesting creates multiple schedule *scopes* (outer macro-step, inner SCC
sub-iteration, possibly sub-step / rollback below that), with two failure shapes: (a) an
inner schedule silently runs under the wrong policy; (b) the *same* producer's output has
different "settled" meanings to different consumers at different depths — intra-SCC it is
fresh mid-iteration, to a downstream macro consumer it is only settled at convergence.

- **M2a — scope by edge, not by executor.** Each edge carries a static *scope tag* (its
  innermost scope, decided at build from the SCC / sub-step decomposition). Correctness
  never depends on which executor is currently active. The R5 single scalar generalizes to
  a small phase *stack* (realistic depth ≤ 3: macro → SCC-iteration → sub-step). Read cost
  is an indexed lookup by tag (or a pre-resolved per-(edge, frame) plan), not a tree walk.
- **M2b — derive the policy tree from the executor tree, not beside it.** The policy
  hierarchy mirrors the scheduler hierarchy, but is a *projection* of the same build input
  (SCC decomposition + sub-executor config) produced by the same build step. Structural
  disagreement between the two trees becomes impossible — stopping nesting from *recreating*
  the two-sources-of-truth problem at tree level.
- **M2c — no inheritance; completeness check.** Policy assignment is explicit per edge /
  region; a scope that reaches an unassigned edge is a *build error*. This kills the
  "silently inherits the wrong policy" failure for the cost of a build-time sweep.
- **M2d — per-scope frontiers + visibility rule.** Commits are tagged with the scope that
  produced them; a read in frame X sees the max commit whose scope is a prefix of X.
  Cross-scope reads resolve against the outer frame: a downstream macro consumer of an SCC
  node sees the *converged* value, never mid-iteration data. One mechanism (monotonic
  per-scope commits + a prefix-visibility rule) generalizes to per-producer watermarks at
  the leaves.

**Note — nesting resurfaces D16, not a new failure class.** A consumer with edges from two
*different* frames hits joint-freshness again — the standing "explicitly support, or
explicitly forbid" decision of **F**. Residual costs: a practical depth limit (state ≤ 3),
and plan caches multiplied by the scope count (small).

---

## The combined shape

Both weakness families reduce to the same pattern:

> **Pin down structure at build time (scope tags, policy-tree derivation, plan templates,
> completeness checks); leave exactly one mutable runtime fact (the committed frontier);
> make everything else a pure function of it.**

The two fix-sets compose cleanly: scope tags feed the phase stack; the phase stack feeds
rule/template selection; the frontier gates every target. Net assessment (honest):
*two-sources-of-truth is largely eliminable* (frontier as sole truth + clamping + staleness
signature); *nesting is structurally eliminable* (derived tree + explicit tags) with a
small explicit residual (depth limit, D16 re-surface at cross-frame consumers).

The residuals above remain open — the mitigations are presented as structure, not as a
decision to adopt any of them.

---

## Read-target resolver — the concrete form of C, and the sub-executor question

A step back from pluggable policies and nesting machinery: is there a *narrower* utility
that captures most of the value?

### The utility

A single, per-connection read-target resolver:

```
resolve(edge, phase, commits, scope_tags) → read-target
```

where `read-target` is a **committed frontier version** (not a raw search time). It has
graph access, and from that plus schedule facts it answers "what is the latest /
appropriate / valid target this connection should read?" — the "correct" per graph role:

| Edge role | Correct read target |
|---|---|
| Stale (Jacobi-like) | latest committed ≤ step-bound |
| Fresh in-sweep | this frame's producer commits — *legal only if* the schedule grants happens-before; otherwise it reports "not yet" |
| Inside SCC region | this iteration's re-iteration frontier (fresh within region) |
| Cross-branch | global-min committed frontier, or per-producer watermark (F) |
| Delay edge | committed frontier − lag |
| Cross-frame | outer frame via prefix-visibility (M2d) |

Why returning a **frontier version** matters: the read side then performs a **direct index
address, no scan**. This is design **C's "query refinement" made concrete and graph-aware** —
the resolver *is* the "static AccessWindow.phase rule" C gestures at, centralized with graph
access. It composes with the index mechanisms (B/G/C) instead of re-opening the scan debate.

**Pitfalls (real, not new):**
- **Performance is the whole claim.** If the resolver runs per read, it *is* the status-quo
  scan renamed — the "indirection is free only if build/execute split holds" trap. It must
  run once per phase change and emit a cached concrete target that the read loop executes.
  (M1a's "phase selects rules, frontier gates targets" is exactly this split.)
- **It must not invent happens-before.** Freshness is legal only if the schedule *produced*
  that fact; the resolver reports it, never asserts it. Otherwise D3/D17 re-enter through
  the front door.
- **Two-sources-of-truth reappears** — but the M1b/M1c mitigations apply verbatim: the
  resolver is a pure function; the committed frontier is the single mutable input.
- **It does NOT decide who runs when.** It consumes schedule facts; it does not produce
  them. So it is strictly simpler than architecture I — no pluggable policies, no product
  space. It covers only the read side; the write side (who commits, when) stays in the
  executors.

**Verdict.** Sound and narrow — and it is arguably the cheapest repair for the current
mis-split (the builder pins `StartTime`, the executor sets `input_time`, the connection owns
the mode: a *three-way* split that is exactly why Seidel in-sweep freshness is latent, not
real). The resolver unifies "what should this edge read" in one pure function. It is a
*reduction* of architecture I, not a competitor to it.

### The sub-executor question: is nesting worth it?

**What nesting uniquely buys:** genuine recursion — an inner *independent* schedule (and
policy, and scope) inside an outer one. Its only unique value is **cross-algorithm
composition**: an outer Parallel-Jacobi hosting an inner Seidel/SCC region that iterates to
convergence with fresh reads while everything outside is still a Jacobi batch — the full
UC-5 realization.

**What it costs:** phase stacks, per-scope frontiers, plan-tree derivation, build-time
completeness sweeps, D16 at cross-frame edges — the M2a–d machinery.

**The honest hinge:** SCC *semantics* (iterate a feedback region within a step) do **not**
require nesting. A **flat schedule that knows its SCC regions** iterates a region with fresh
reads and commits the region at convergence through the same prefix-visibility rule (M2d) —
one schedule with regional knowledge, no recursive sub-executor. Nesting only earns its
keep if inner schedules are a *different algorithm* while the outer is running:

- If the target models never need Jacobi-outside/Seidel-inside *simultaneously* →
  flat-regional + scope tags captures UC-5 at a fraction of the complexity; nesting is an
  overrun.
- If they do → nesting earns its keep, and M2a–d are the price of admission.

The repo already tilts this way: `LoopAware` *already owns a sub-executor*. So the real
decision is **keep-and-isolate vs refactor-out** — and that turns on whether the inner
schedule is ever a *different algorithm*, rather than the same algorithm iterating. If it is
always the same algorithm iterating (as the `loop_aware` setup suggests), it is a
flat-regional behavior wearing a recursive costume, and cutting the recursion reduces state
space now.

### Why the resolver must not presuppose nesting

The sub-executor decision can only be deferred if the resolver's correctness does not depend
on nesting existing. Therefore the resolver's **contract is stated against a non-nested
(flat) schedule**:

- Its inputs are **scope tags** (build-time graph properties on edges) plus **schedule
  facts** — never a notion of an active executor tree.
- It resolves identically whether the schedule is a flat regional schedule or a genuinely
  nested tree, because scope tags and frontier facts carry all the information it needs.
- Consequently a future sub-executor, if adopted, must be expressible **as a flat projection
  of the same inputs** (scope tags + per-region frontiers) — not as a new resolver contract.

This makes sequencing safe and establishes the claim: **the resolver is built and verified
on the flat basis first; the sub-executor adoption is a later, evidence-driven decision that
is not allowed to change the resolver's contract.** If the nesting decision is made to keep
a sub-executor, it is a *schedule-shape* decision layered on top of an unchanged resolver —
never a reason to extend it.

A concrete C++ mockup of the resolver lives in its own implementation document. The initial
version is **streamlined**: the constructor takes the simulation graph and derives per-model
status centrally (`ModelStatus`), so connectors/connections and frontiers are reached through
one object:

> **See [`data_access_read_target_resolver.md`](data_access_read_target_resolver.md)** —
> the initial interface mockup: graph-injected constructor, centralized per-model status,
> frontier-clamped read-time resolution, read-loop + executor sketches, constraint mapping,
> and the items intentionally deferred (plan caching, edge roles/scopes, joint-frame vectors).

---

## Open questions (no lean)

- Does the policy own the storage layout, or only the addressing resolution? (Decides
  whether A can genuinely be a policy instance.)
- Is plan syntax standardized across policies so the hot path is policy-agnostic? (The
  entire "indirection is free" claim rests on this.)
- Compatibility: build-time declared capability, graph-asserted, or runtime-probed?
- Is the access graph a *materialized alternative graph* (extra copy nodes, delay buffers
  as real nodes) or only an addressing function over the same edges?
- Do nested executors carry their own policy instance, or inherit the outer one?
- Is the capability policy (which mechanisms are allowed where) stated per edge, per
  subgraph, or per executor-family?
- Does M1a's frontier-clamp degrade fresh edges to a stale read, and is that shortfall
  surfaced only through an observability projection — acceptable, or must it be an error?
- Is the phase stack in M2a truly bounded at ≤ 3 (macro → SCC-iteration → sub-step), or do
  real graphs nest deeper?
- For M2d's prefix-visibility rule: is per-scope commit tagging a new cost, or does it
  collapse into existing per-producer watermarks (F) at the leaves?
- **Sub-executor fate (open, evidence-driven):** do target models need cross-algorithm
  composition (Jacobi-outside / Seidel-inside simultaneously)? Decides keep-and-isolate vs
  flat-regional refactor-out for the existing `LoopAware` sub-executor. The read-target
  resolver must land first and must not change with this decision.
- **Resolver scope:** is the resolver (a) a concrete, graph-aware component that actually
  gets built (the "reduction of I" path), or (b) only an articulation of C's rule that stays
  conceptual?
- **Resolver output form:** committed frontier version only (direct index, no scan) — is
  there any case that genuinely needs a raw search time returned instead?

## Traceability

- **Mechanisms it composes**: see `data_access_and_scheduling_design.md` (A–H, cross-critique,
  the existing lean on A/C/B).
- **Problem-side**: see `data_access_and_scheduling_problem_space.md` (axes — especially 3
  and 5 — use-cases, difficulties P1–P18, open questions).
- **Current-behavior corrections**: see `data_access_and_scheduling.md` (D series, review
  corpus, D6 interface analysis — R3/R5 are the direct predecessors of this candidate).
- **Open questions above** map to problem-space open questions 6 and 10, and design-space
  gaps D2/D10/D11/D16/D17/UC-5/UC-6/UC-10.
- **Mitigations above** reference mechanisms **C** (watermark clamp, M1a), **E** (frontier
  re-load, M1c), **G** (rule/template selection, M1a, M2a), **F** (per-scope watermarks,
  M2d), and the D17 acquire/compare (reused in M1c).
- **Read-target resolver** is the concrete form of design **C's** AccessWindow rule,
  composed with **B/G** (direct index from a committed frontier version); it is a reduction
  of this architectural candidate, not a competitor. Its non-nesting contract ("resolved on
  a flat schedule; scope tags + frontier facts"), which in turn makes nesting deferrable.
- **Interface mockup**: see `data_access_read_target_resolver.md` — the concrete C++ sketch
  (`EdgeSpec`, `Frontier`, `AccessPlan`, `ReadTargetResolver`, usage + constraint-mapping
  table). Split out so this doc stays concept-only.

### Key open decision (stands apart)

The sub-executor question — **keep-and-isolate vs flat-regional refactor-out** for the
existing `LoopAware` sub-executor — is not resolvable by architecture alone: it depends on
whether target models require cross-algorithm composition. It is deferred to model evidence,
*after* the non-nested read-target resolver lands and without the resolver being allowed to
change with it.