# Regressions and Known Issues
<!-- Layer: 04-verification -->
<!-- Stable ID: VER-REGRESSIONS-001 -->

## Description

This artifact tracks known regressions and unresolved issues in the verification suite. Each entry includes affected component, root cause (if known), status, and workaround.

## Active Issues

| ID | Affected Fixture | Description | Root Cause | Status | Workaround |
|---|---|---|---|---|---|
| REG-001 | `signal_sine_gain_add/baseline` | Runtime emits different start values than fixture declares | Unknown — under investigation | Open | Marked as strict xfail in pytest |
| REG-002 | `dcmotor/baseline` | Runtime emits different start values than fixture declares | Unknown — under investigation | Open | Marked as strict xfail in pytest |

## Tracking Method

- Issues are tracked as strict pytest xfail markers with rationale strings
- XPASS detection alerts when a test starts passing
- This document complements pytest markers with structured root cause tracking
- Each issue SHOULD have a corresponding GitHub issue for resolution tracking

## Resolution Policy

- New regressions must be documented here or in pytest xfail markers before merging
- Fixed regressions should have the xfail marker removed and this doc updated
- Root cause investigations should reference the GitHub issue number

## Traceability

- Backward: Traces to decision VD-002 (xfail tracking method) in `product-breakdown/04-verification/decisions/`.
- Sources: `tests/README.md` (xfail section), Python test markers.