---
id: INFO-010
type: info
title: Context view
summary: SSP4SIM system boundary, external actors, external systems, and trust boundaries
date: 2026-11-11
status: current
---

# Context view

SSP4SIM is a simulation engine: consumes SSP archives, loads FMI 2.0 FMUs, produces simulation results. Direct path from SSP input to executable run, not a full simulation environment.

- **Actors:** CLI invokes `sim_app` with a JSON config file (exit codes, stdout/stderr); Python caller imports `pyssp4sim` and calls the simulator API; C API caller links the shared library (error code returns). Concrete entry points are code (`INFO-016`).
- **Input systems:** SSP 1.0 archive or unpacked directory (filesystem); FMI 2.0 co-simulation FMU binaries (dlopen/dlsym via the fmi4c adapter, Linux shared libraries).
- **Output systems:** local result database (SQLite WAL) and CSV export (filesystem); log sinks terminal, file, JSON, cutelog. Remote result ingestion via InfluxDB Line Protocol is future output, out of scope for current releases.
- **Trust boundaries:** FMU vendor libraries run unsandboxed with the caller's privileges; config files and SSP archives treated as trusted input — structural validity checked, no security validation.
