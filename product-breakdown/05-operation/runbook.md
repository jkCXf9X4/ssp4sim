# Runbook
<!-- Layer: 05-operation -->
<!-- Stable ID: OPS-RUNBOOK-001 -->

## Description

This artifact documents common operational procedures for running SSP4SIM simulations — startup, verification, and remediation.

## Startup

1. Ensure the SSP archive or directory exists and is readable
2. Ensure the JSON configuration file is valid (see `docs/configuration.md` for schema)
3. Ensure FMU binaries in the SSP have executable permissions (Linux shared libraries)
4. Run: `sim_app /path/to/config.json`
5. Alternative: `pyssp4sim /path/to/config.json` (if wheel is installed)
6. Within Python: `pyssp4sim.Simulator("/path/to/config.json").init().simulate()`

## Verification

After a successful run, verify:
- Simulation completed without ERROR or CRITICAL log messages
- Output files exist at the configured working directory
- CSV output has the expected columns and row count
- DuckDB output has per-storage tables populated

## Common Issues

| Issue | Symptom | Resolution |
|---|---|---|
| FMU not found | "file not found" or dlopen error | Check SSP path, FMU binary location, and executable permissions |
| Config error | Parsing failure on startup | Validate JSON syntax and key names against docs/configuration.md |
| Simulation hang | No output for extended period | Check log for last successful step; check for FMU timeout or recorder backpressure |
| Wrong results | CSV differs from expected | Check signal connections in SSD; compare with reference SSP results |
| ctest failure | Test binary fails in CI | Run test binary directly: `./build/tests/lib/ssp4sim_tests` |

## Traceability

- Backward: Deployment topology in `product-breakdown/05-operation/deployment-topology.md`.
- Sources: `docs/usage.md`, `docs/configuration.md`.
