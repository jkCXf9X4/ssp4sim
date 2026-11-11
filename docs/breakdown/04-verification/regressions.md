---
id: INFO-031
type: info
title: Regressions
summary: Known issues tracked as pytest xfails with rationale; one open regression (REG-003), two recorded ones fixed and their markers removed
date: 2026-11-11
status: current
---

# Regressions

One open regression; two recorded ones are fixed and their xfail markers removed.

- **REG-003** (`embrace/baseline/model`): open. BC FMU fails at simulator init with `RuntimeError: Failed to write string value to FMU` (`fmi4c_adapter.cpp:613`, "failed to write_string, vr BC"). Root cause: new fixture BC FMU lacks `resources/data/CONOPS.csv`; the `csv_path` String start value (vr 0, `data/CONOPS.csv`) is pushed to the FMU on init in old and new fixtures alike, and `fmi2SetString` fails when the file is absent. Not a layout issue: direct `pyssp4sim.Simulator` run reproduces; old submodule commit `f484677` simulates with the same build. xfail marker in `test_reference_sweep.py` (strict=False).
- **REG-001** (`signal_sine_gain_add/baseline`): fixed; runtime start-value mismatch resolved by correcting the namespace in `ssp1_ssd.toml` (ssv to ssd); xfail marker removed, test passes.
- **REG-002** (`dcmotor/baseline`): fixed; runtime start-value mismatch resolved by adding component-level `<ssd:ParameterBindings>` traversal to `get_parameter_bindings()` in `ssp.cpp`; xfail marker removed, test passes.
- **Tracking method:**
  - Known issues are marked with pytest xfail markers carrying rationale strings (REG-003 and the nested-override case use strict=False, so XPASS flags the fix without failing the suite).
  - XPASS detection alerts when a marked test starts passing.
  - This leaf complements markers with structured root-cause tracking.
  - Each issue tracks a corresponding GitHub issue.
  - Not a regression but open: `signal_nested_parameter_bindings` is xfail-marked (strict=False) because the root SSV+SSM override of the nested target `inner.sine.amplitude` (= 2.0) is ignored and the inline value 1.0 wins. Nested start-value propagation is not yet supported; tracked as IMP-042.
- **Resolution policy:**
  - New regressions are documented here or in an xfail marker before merging.
  - A fixed regression has its xfail marker removed and this leaf updated.
  - Root-cause investigations reference the GitHub issue number.
