---
id: INFO-027
type: info
title: Acceptance Criteria
summary: Release and quality gates: all tests pass, xfails documented, breaking changes carry test updates
date: 2026-11-11
status: current
---

# Acceptance Criteria

A release or quality pass is accepted when the release gate and the quality gate both hold.

- **Release gate:**
  - All C++ unit and integration tests pass, zero failures.
  - Python workflow tests pass, documented strict xfails excluded.
  - Known xfail list is maintained and reviewed per release.
- **Quality gate:**
  - No new xfail without documented rationale (issue ID or root-cause note).
  - No regression in existing xfail count without documented resolution.
  - Breaking changes to schema, API, or behavior ship with test updates.
- **Breaking change:** a change that alters the simulation output format, CLI interface, Python API, or configuration schema such that consumers must update.
- **Documented rationale:** an xfail annotation references an issue number or a root-cause description.
- **Related leaves:** coverage targets in `INFO-029`; CI stage gates in `INFO-028`.
