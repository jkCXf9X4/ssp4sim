---
id: INFO-039
type: info
title: Review model
summary: How definition state and changes are scrutinized before they are trusted
date: 2026-11-11
status: current
---

# Review model

Review scrutinizes the breakdown and the changes that realize it; verification proves claims separately.

- **Mechanical gates:** `pb check --strict` and `pb node-size --strict` pass before a change lands; failures are blockers.
- **Severity:** blocker is wrong or unverifiable state presented as current; major is misrouted, duplicated, or stale fact; minor is wording drift.
- **Routing:** a finding's home is the layer owning the defective fact; the finding carries evidence, the leaf carries the corrected fact.
- **Routes:** edit the owning leaf for a wrong fact, an IMP for new work, a decision record for a baseline change, a tombstone for obsolete material.
- **Closure:** owning leaf updated, gates green, traceability row updated when a claim or artifact moved.
- **Cadence:** review the touched nodes per change, a full sweep per release before registers publish.
