# Acceptance Criteria
<!-- Layer: 04-verification -->
<!-- Stable ID: VER-ACCEPTANCE-001 -->

## Description

This artifact defines the conditions for product acceptance at each verification gate. This initial version is extended by IMP-008 with coverage targets and CI stage detail.

## Release Gate

- All C++ unit and integration tests pass (0 failures).
- Python workflow tests pass (documented strict xfails excluded).
- Known xfail list is maintained and reviewed per release.

## Quality Gate

- No new xfails introduced without documented rationale (issue ID or root cause note).
- No regression in existing xfail count without documented resolution.
- Breaking changes (schema, API, behavior) include corresponding test updates.

## Definitions

- **Breaking change**: Any change that alters the simulation output format, CLI interface, Python API, or configuration schema in a way that requires consumer updates.
- **Documented rationale**: An xfail annotation MUST include a reference to an issue number or a root cause description.

## Traceability

- Backward: Traces to requirements in `product-breakdown/01-product/requirements/`.
- Sources: `tests/README.md`, current test practices.
