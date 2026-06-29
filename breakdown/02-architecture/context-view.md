# Context View
<!-- Layer: 02-architecture -->
<!-- Stable ID: ARCH-CONTEXT-001 -->

## Description

This artifact describes the SSP4SIM system boundaries, external actors, and external systems. It answers: what is inside the system, what is outside, and how do they interact?

## System Boundary

SSP4SIM is a simulation engine that consumes SSP archives, loads FMI 2.0 FMUs, and produces simulation results. It is not a full simulation environment — it focuses on a direct path from SSP input to executable run.

## External Actors

| Actor | Description | Interaction |
|---|---|---|
| **User (CLI)** | Invokes `sim_app` with a JSON config file | Command-line invocation, exit codes, stdout/stderr output |
| **Python Caller** | Imports `pyssp4sim` and calls Simulator API | Python module import, method calls, exception handling |
| **C API Caller** | Links against shared library, calls C API | C function calls, struct parameters, error code returns |

## External Systems

| System | Description | Protocol |
|---|---|---|
| **SSP Archive** | Input — SSP 1.0 archive (.ssp) or unpacked directory | Filesystem read, zip extraction |
| **FMU Shared Libraries** | Input — FMI 2.0 co-simulation FMU binaries | dlopen/dlsym (via fmi4c adapter), Linux shared library |
| **Local Result Database** | Output — simulation results in local database format; currently SQLite WAL | SQLite C API, filesystem write |
| **CSV Files** | Output — portable export of simulation results in CSV format | Filesystem write |
| **Remote Result Database** | Future output — multi-source simulation result ingestion via InfluxDB Line Protocol | Out of scope for current releases |
| **Log Sinks** | Output — terminal, file, JSON, cutelog | Filesystem write, network (cutelog) |

## Trust Boundaries

- FMU vendor libraries are loaded via dlopen with caller's privileges — no sandboxing
- Config files are read from the filesystem — treated as trusted input
- SSP archives are parsed from the filesystem — structural validity checked, no security validation

## Traceability

- Backward: Traces to product scope in `product-breakdown/01-product/scope.md`.
- Sources: Entry points (`public/ssp4sim_app/`, `public/python_api/`), `lib/public_include/simulator_c_api.h`.
