---
id: INFO-037
type: info
title: Runbook
summary: Day-to-day operations — startup, post-run verification, and remediation of common issues
date: 2026-11-11
status: current
---

# Runbook

Common operational procedures for running SSP4SIM simulations: startup, verification, and remediation.

- **Startup:**
  1. Ensure the SSP archive or directory exists and is readable.
  2. Ensure the JSON configuration file is valid per the configuration guide.
  3. Ensure FMU binaries in the SSP have executable permissions (Linux shared libraries).
  4. Run `sim_app /path/to/config.json`.
  5. Alternative: `pyssp4sim /path/to/config.json` (when the wheel is installed).
  6. Within Python: `pyssp4sim.Simulator("/path/to/config.json").init().simulate()`.
- **Verification after a run:**
  - Simulation completed without ERROR or CRITICAL log messages.
  - Output files exist at the configured working directory.
  - CSV export has the expected columns and row count.
  - Local database output has per-storage tables populated.
- **Common issues:**
  - FMU not found ("file not found" or dlopen error) — check SSP path, FMU binary location, and executable permissions.
  - Config error (parsing failure on startup) — validate JSON syntax and key names against the configuration guide.
  - Simulation hang (no output for an extended period) — check the log for the last successful step, FMU timeout, or recorder backpressure.
  - Wrong results (CSV differs from expected) — check signal connections in the SSD and compare with reference-SSP results.
  - ctest failure in CI — run the test binary directly (`./build/tests/lib/ssp4sim_tests`).
