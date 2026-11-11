---
id: INFO-020
type: info
title: Data access and scheduling — design space
summary: Candidate designs A–H for deterministic data access, the current design's known defects, and the non-binding lean
date: 2026-11-11
status: current
---

# Data access and scheduling — design space

Three orthogonal candidate designs plus five further candidates for deterministic producer-read in Gauss–Jacobi / Gauss–Seidel execution; analysis only, no binding decision.

- **Problem:** how connection reads of producer data stay deterministic when execution is parallel and the schedule mixes Jacobi and Seidel styles; the read path is hot — once per connected model per step. Problem-side detail: `INFO-021`.
- **Current system, unpolished:** per-edge scan plus mode switch plus get-item verification on every model pre(); known review defects: D2 torn read and first-commit underflow; D11 sub-step mid-window commits; D13 t=0/init frontier; D15 raw-copy double-free on slot reuse; D16 no joint-freshness reconcile; D17 no stated memory-ordering rule.
- **Three primary designs:**
  - A push / publish-at-commit: producer copies committed values into each consumer's target slot at commit; read = load + one acquire; needs every consumer's layout; history/lag reads need a second path; wasted work when consumers are few.
  - B compile-time slot table: `slot(w) = w mod capacity` resolved at build; violations become compile errors; unconditional schedules only — sub-step re-entry, rollback, and convergence adaptation leave the table stale mid-run.
  - C query-refined committed watermark: O(1) head-derived index, static `AccessWindow.phase` rule, watermark as the only truth, type-aware copies, verification off in release; works under any schedule; smallest migration.
- **Invariant:** none of A/B/C escapes the committed watermark + memory-ordering requirement (D17); where history must exist, A doubles storage and B collapses — the pull model (current/C) is the only one that compounds cleanly.
- **Further candidates:** D schedule-discipline (level-sync, generation double-buffer); E immutable append-only commit log (rollback = pointer re-wind, needs reclaim); F per-consumer version vector (answers D16, O(fan-in)); G phase-keyed plan template set (adaptive B, presence check survives); H token-skip zero-order-hold copy suppression (orthogonal, cheap).
- **Lean (first-class, non-binding):** A for no-history edges, C fallback for lagged/history/adaptive parts, B-optimized fast path for known-unconditional subgraphs; the no-history class is not closed under adaptation, so the A/C seam stays open.
- **Candidate I:** the pluggable access-policy architecture is documented separately (`INFO-022`).
