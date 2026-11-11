---
id: INFO-042
type: info
title: Data access and scheduling — known difficulties
summary: The eighteen known difficulties (P1–P18) in the data-access problem space
date: 2026-11-11
status: current
---

# Data access and scheduling — known difficulties

Eighteen difficulties (P1 to P18) the design space must answer.

- **Storage identity and addressing:** P1 ring wrap loses slot identity without a generation/epoch check — existence is not validity; P2 offsets from a live write pointer race under parallel producers (torn reads, silent stale/underflow at first batch); P6 reading `l` back needs `l+1` areas — one-area storages silently serve newest, usable window smaller than nominal; P15 slot re-use under raw copy corrupts complex values — re-writes must be type-aware.
- **Ordering and frontiers:** P3 "newest" is schedule-defined, never storage-defined — fresh reads racy, serial and parallel may disagree at step 1; P4 one global frontier couples branches to the slowest producer; P5 per-producer frontiers must not desync recorders — a lagging recorder diverges exported from read timelines; P13 t=0 has no defined read frontier; P14 irrevocable frontiers conflict with rollback and checkpoints.
- **Joint freshness and recording:** P16 no cross-producer reconciliation — a consumer differencing two sources can read step k and k−3; P12 a commit visible to export before consumers may legally read it — tears determinism and recorded accuracy.
- **Scheduler and policy boundaries:** P7 schedulers exchange only a fixed step descriptor — sampling-policy boundary undefined; P8 time is the only schedule-invariant anchor; P10 correctness argued per parallel variant, stubs carry no guarantee; P11 sub-steps break the one-push-per-step identity; P9 mixed semantics hurt interpretability (lowest priority).
- **Pre-computation tension:** P17 a static plan is cheapest but the schedule is dynamic — build-time push assumes a schedule shape parallelism cannot guarantee; P18 a pre-resolved address can silently serve wrong data when rollback, sub-steps, or wrap invalidate build assumptions — nothing left in the hot path to notice.
