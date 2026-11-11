---
id: INFO-031
type: info
title: Regressions
summary: Known issues tracked as strict pytest xfails with rationale; the two recorded regressions are fixed and their markers removed
date: 2026-11-11
status: current
---

# Regressions

The suite currently holds no open regressions; both recorded ones are fixed and their xfail markers removed.

- **REG-001** (`signal_sine_gain_add/baseline`): fixed; runtime start-value mismatch resolved by correcting the namespace in `ssp1_ssd.toml` (ssv to ssd); xfail marker removed, test passes.
- **REG-002** (`dcmotor/baseline`): fixed; runtime start-value mismatch resolved by adding component-level `<ssd:ParameterBindings>` traversal to `get_parameter_bindings()` in `ssp.cpp`; xfail marker removed, test passes.
- **Tracking method:**
  - Known issues are marked with strict pytest xfail markers carrying rationale strings.
  - XPASS detection alerts when a marked test starts passing.
  - This leaf complements markers with structured root-cause tracking.
  - Each issue tracks a corresponding GitHub issue.
- **Resolution policy:**
  - New regressions are documented here or in an xfail marker before merging.
  - A fixed regression has its xfail marker removed and this leaf updated.
  - Root-cause investigations reference the GitHub issue number.
