---
id: INFO-022
type: info
title: Data access and scheduling — access-policy architecture
summary: Candidate I — data access as a pluggable strategy beside the executor; candidacy, mitigations, open questions
date: 2026-11-11
status: current
---

# Data access and scheduling — access-policy architecture

Candidate I is an architectural option in the data-access space: it answers where the sampling-policy decision lives and how policies are selected and composed; it adds no speed or safety of its own.

- **Candidacy:** a candidate, not a decision — kept separate from the mechanism designs (A–H, `INFO-020`) because it differs in kind; every aspect of its behavior still delegates to a mechanism, so its value is the sum of the A–H properties it is allowed to compose.
- **Idea:** make data access a pluggable strategy chosen beside the executor, splitting the execution graph (who runs when) from the access graph (per-connection source, addressing, freshness); a connection asks an access policy, "resolve edge e at phase p", which returns the index or time to read.
- **Mechanisms as policy instances:** A eager-push; B static-plan (fixed cadence); C committed-watermark; D generation-flip; E immutable-log; G phase-keyed template; H copy-suppression as an additive layer any policy can carry.
- **What it buys:** independent replaceability of executor and access semantics; access-only graph rewrites (delay buffers, fresh-to-stale downgrades) that leave execution untouched; per-policy verification of declared guarantees at bind (fail-fast at build); a clean plan-build / plan-execute split so policy logic runs once per phase change.
- **Costs:** indirection is free only if the build/execute split holds — per-read consultation is the status quo under a new name; two sources of truth (executor progress vs policy view), mitigated by the rule that the executor is the sole writer of schedule facts and the policy is a pure function of them; executor x policy compatibility must be declared or bind-checked; nesting — LoopAware hosts a sub-executor, so policies must nest; the storage-ownership boundary decides whether A can genuinely be a policy instance.
- **Mitigations:** M1a phase selects rules, frontier gates targets — timing drift degrades to a safe stale or shortfall read, never a torn one; M1b policy holds zero mutable state, the committed frontier is the sole mutable truth via one channel (executor-issued commits); M1c stale-plan signature — capture the assumed frontier at plan build, rebuild or fail fast on mismatch; M2a scope tags per edge (innermost scope, depth at most 3); M2b policy tree derived from the executor tree; M2c explicit per-edge/region assignment, an unassigned edge is a build error; M2d per-scope commit tagging with prefix visibility — a downstream macro consumer sees a converged SCC value, never mid-iteration data.
- **Combined shape:** pin structure at build time (scope tags, derived policy tree, plan templates, completeness checks); leave exactly one mutable runtime fact, the committed frontier; make everything else a pure function of it. Residuals stay open.
