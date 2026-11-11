---
id: INFO-043
type: info
title: Data access and scheduling — open questions
summary: The unresolved problem-side questions the data-access design space must answer
date: 2026-11-11
status: current
---

# Data access and scheduling — open questions

Open problem-side questions in the data-access design space.

- **Determinism contract:** same schedule implies same result (weak) vs any schedule implies same result (strong); evidence so far suggests strong is unachievable in general, so the contract must be scoped and golden tests must state whether they lock serial results.
- **Frontier semantics:** rollback / checkpoint / failed-step resume against a frozen committed frontier; the t=0 initialization rule for reads before any commit; recorder visibility — when a push is committed versus read, and whether export flushes before the read frontier advances.
- **Mapping:** how "one step back" maps to stored areas when sub-steps break the one-push-per-step identity; multi-rate stepping.
- **Authority:** who decides sampling policy — executor or model — and via what boundary, without leaking per-connection detail into the scheduler.
- **History-window enforcement:** how the size bound (P6) is guarded — configuration, build-time assertion, or runtime clamp-and-warn.
- **Joint freshness:** whether a consumer can depend on several producers being at the same frontier, and how that is expressed or forbidden (P16).
- **Pre-computation boundary:** how much of the per-edge read decision collapses into a fixed build-time plan, whether edges are explicitly classified as fully static versus schedule-informed, and how the auditable statement survives hot-path unrolling.
- **Unlinked-read contract:** "no explicit edge implies stale-only via the committed frontier" — enforced as a connections-registry `unlinked` edge or a `validate()` build-time assertion; and what an unlinked producer that has never committed resolves to.
