---
id: INFO-040
type: info
title: Data access and scheduling — read-target resolver
summary: The graph-aware narrowing of candidate I, the sub-executor question, and policy-side open questions
date: 2026-11-11
status: current
---

# Data access and scheduling — read-target resolver

A narrower utility than the full policy architecture captures most of its value; the sub-executor fate stays open and evidence-driven.

- **Utility:** one per-connection resolver, `resolve(edge, phase, commits, scope_tags)` returning a read-target — a committed frontier version, not a raw search time; the read side then does a direct index address, no scan. This is design C's query refinement made concrete and graph-aware, composing with B/G/C (`INFO-020`).
- **Per-edge roles:** stale reads the latest committed at or before the step bound; fresh in-sweep reads this frame's producer commits, legal only if the schedule grants happens-before, else reports "not yet"; inside an SCC it reads the iteration frontier; cross-branch it reads the global-min committed frontier or a per-producer watermark; delay edges read frontier minus lag; cross-frame it reads the outer frame via the prefix-visibility rule.
- **Pitfalls:** performance is the whole claim — the resolver must run once per phase change and emit a cached concrete target, or it is the status-quo scan renamed; it must not invent happens-before — freshness is legal only if the schedule produced that fact; it consumes schedule facts, never produces them — the write side (who commits, when) stays in the executors.
- **Verdict:** sound and narrow — a reduction of candidate I, not a competitor; it unifies "what should this edge read" in one pure function and is arguably the cheapest repair for the current three-way split (builder pins start time, executor sets input time, connection owns the mode).
- **Sub-executor question (open):** SCC semantics do not require nesting — a flat schedule that knows its SCC regions iterates with fresh reads and commits at convergence through the same prefix-visibility rule; nesting earns its keep only when the inner schedule is a different algorithm; `LoopAware` already owns a sub-executor, so the decision is keep-and-isolate vs flat-regional refactor-out, decided by whether cross-algorithm composition is needed.
- **Resolver contract:** stated against a non-nested schedule — inputs are scope tags plus frontier facts, never an active executor tree; a future sub-executor must be a flat projection of the same inputs and must not change the resolver's contract.
- **Open questions:** does the policy own storage layout or only addressing; is plan syntax standardized across policies so the hot path is policy-agnostic; is capability declared at build, asserted in the graph, or probed at runtime; is the access graph a materialized alternative graph or an addressing function over the same edges; does a nested executor carry its own policy or inherit the outer one; does the M1a clamp degrade fresh edges to stale and is that shortfall surfaced by observability or as an error.
