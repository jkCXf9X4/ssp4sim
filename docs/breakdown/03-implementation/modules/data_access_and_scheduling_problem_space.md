---
id: INFO-021
type: info
title: Data access and scheduling — problem space
summary: The problem, vocabulary, and axes the design space must satisfy
date: 2026-11-11
status: current
---

# Data access and scheduling — problem space

The problem space around how connections read producer data in Gauss–Jacobi / Gauss–Seidel execution, recorded solution-free.

- **Problem:** can producer data be read deterministically when execution is parallel, the schedule mixes Jacobi and Seidel styles, and the schedule is long/deep; stored data is addressed by time (search) or by index (direct), and the acceptable answer must be heavily pre-computable at graph build because the read path is hot (once per connected model per step).
- **Vocabulary:** area (one time-versioned record of a producer's output); head (the newest visible data, the live write pointer); committed frontier/watermark (a frozen read boundary — reads resolve against it, never against live writes); lookback (committed steps back a connection reads); phase/epoch (schedule identity carried by the executor); connection (one typed edge from source to consumer); SCC (feedback nodes that iterate inside a step).
- **Axes:** what to sample (value style); how to address storage (time vs index); who decides and when (static per-connection at build vs schedule-driven runtime frontier, macro-step vs sub-step); visibility and persist-what (what recording/export sees stays consistent with legal reads); pre-computability and hot-path cost (statically decidable at build vs schedule-informed).
- **Mixed semantics:** one schedule mixes stale (Jacobi-like), fresh (Seidel-like), and delayed edges per connection; the core question is whether a single read model expresses all three deterministically.
- **Pre-computation tension:** at build some facts are statically decidable per edge (producer storage, value style, lookback rule, even the physical ring location for fixed acyclic graphs); others are not (which frontier is legal now, fresh-edge legality, t=0, rollback); the goal is maximum static collapse with a small schedule-informed delta that never pays the general-case cost on the hot path.
- **Detail leaves:** read behaviors the design must express (`INFO-041`); known difficulties (`INFO-042`); unresolved questions (`INFO-043`).
