---
id: INFO-031
type: info
title: Regressions
summary: Known issues tracked as pytest xfails with rationale; the three recorded regressions are fixed and their markers removed
date: 2026-11-11
status: current
---

# Regressions

All three recorded regressions are fixed and their xfail markers removed.

- **REG-003** (`embrace/baseline/model`): fixed. BC FMU failed at init with `RuntimeError: Failed to write string value to FMU` (`fmi4c_adapter.cpp:613`). Root cause was upstream, in `ssp_references`: the artifact build emits FMU resource data files, but the repo `.gitignore` rule `*.csv` (added 2026-05-07) silently dropped them from new artifact commits. Embrace's BC is a pyfmu-csv FMU that reads `data/CONOPS.csv` via the `csv_path` String start value, and `fmi2SetString` fails when the file is absent; the old artifact predated the rule and kept the file. Fix: `ssp_references` re-includes csv under any `resources/data/` dir (`.gitignore` negation) and committed the rebuilt artifact; ssp4sim bumped the submodule. Also fixed a sweep-wrapper bug (`param.values` passed as a tuple) that masked the true result.
- **REG-001** (`signal_sine_gain_add/baseline`): fixed; runtime start-value mismatch resolved by correcting the namespace in `ssp1_ssd.toml` (ssv to ssd); xfail marker removed, test passes.
- **REG-002** (`dcmotor/baseline`): fixed; runtime start-value mismatch resolved by adding component-level `<ssd:ParameterBindings>` traversal to `get_parameter_bindings()` in `ssp.cpp`; xfail marker removed, test passes.
- **Tracking method:**
  - Known issues are marked with pytest xfail markers carrying rationale strings (strict=False, so XPASS flags the fix without failing the suite).
  - XPASS detection alerts when a marked test starts passing.
  - This leaf complements markers with structured root-cause tracking.
  - Each issue tracks a corresponding GitHub issue.
  - Not a regression but open: `signal_nested_parameter_bindings` is xfail-marked (strict=False) because the root SSV+SSM override of the nested target `inner.sine.amplitude` (= 2.0) is ignored and the inline value 1.0 wins. Nested start-value propagation is not yet supported; tracked as IMP-042.
- **Resolution policy:**
  - New regressions are documented here or in an xfail marker before merging.
  - A fixed regression has its xfail marker removed and this leaf updated.
  - Root-cause investigations reference the GitHub issue number.
