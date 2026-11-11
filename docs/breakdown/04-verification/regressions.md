---
id: INFO-031
type: info
title: Regressions
summary: Known issues tracked as pytest xfails with rationale; all recorded regressions and feature gaps are fixed and their markers removed
date: 2026-11-11
status: current
---

# Regressions

No open regressions or xfails. All recorded issues are fixed and their markers removed.

- **REG-001** (`signal_sine_gain_add/baseline`): fixed.
- **REG-002** (`dcmotor/baseline`): fixed.
- **REG-003** (`embrace/baseline/model`): fixed.
- **Feature gap** (`signal_nested_parameter_bindings`): closed.
- **Tracking method:**
  - Known issues are marked with pytest xfail markers carrying rationale strings; strict=False so XPASS flags the fix without failing the suite.
  - XPASS detection alerts when a marked test starts passing.
  - This leaf complements markers with structured root-cause tracking.
  - Each issue tracks a corresponding GitHub issue.
- **Resolution policy:**
  - New regressions are documented here or in an xfail marker before merging.
  - A fixed regression has its xfail marker removed and this leaf updated.
  - Root-cause investigations reference the GitHub issue number.