---
title: Verification
summary: How each SSP4SIM claim is proven — acceptance gates, test strategy, coverage, CI stages, regressions, and requirement traceability
---

# Verification

## Contents

<!-- pb:index:start -->
<!-- GENERATED FILE — do not edit. Regenerate with `pb registers`. -->
- **INFO-027** [Acceptance Criteria](acceptance-criteria.md) — Release and quality gates: all tests pass, xfails documented, breaking changes carry test updates
- **INFO-028** [CI Test Stages](ci-test-stages.md) — Three GitHub Actions stages — PR check, merge gate, release validation — each with its own compile, test, and gate
- **INFO-029** [Coverage Targets](coverage-targets.md) — Per-module line-coverage targets and what is intentionally not covered; targets are goals, not current state
- **INFO-030** [Test Strategy](test-strategy.md) — Three-layer test pyramid — C++ unit tests, one C++ smoke test, Python workflow tests — and the layer-boundary rationale
- **INFO-031** [Regressions](regressions.md) — Known issues tracked as strict pytest xfails with rationale; the two recorded regressions are fixed and their markers removed
- **TR-001** [Traceability Matrix](traceability-matrix.md) — Requirement-to-test coverage map — one covered, five partial, one missing, seven untraced
<!-- pb:index:end -->
