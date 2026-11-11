---
id: INFO-007
type: info
title: Glossary
summary: Standardized terms and definitions used across the project
date: 2026-11-11
status: current
---

# Glossary

Shared vocabulary for SSP4SIM and its domain.

- **SSP:** System Structure and Parameterization — standard for packaging co-simulation setups.
- **SSD:** System Structure Description — the XML file describing model topology and connections.
- **SSV:** System Structure Parameter Values — file with parameter value overrides.
- **SSM:** System Structure Model — file mapping parameters to FMU variables.
- **FMU:** Functional Mock-up Unit — a packaged model conforming to the FMI standard.
- **FMI 2.0:** Functional Mock-up Interface version 2.0 — standard for model exchange and co-simulation.
- **Co-Simulation:** FMI mode where each FMU is simulated independently and coupled through input/output connections.
- **Gauss-Jacobi:** execution strategy where all FMUs step concurrently using values from the previous time step.
- **Gauss-Seidel:** execution strategy where FMUs step sequentially, letting updated values propagate within a step.
- **SignalStorage:** in-memory buffer holding typed signal values for one simulation area (`INFO-006`).
- **DataRecorder:** component that copies completed signal updates and writes them to configured output sinks (`INFO-006`).
- **AnalysisGraph:** structural view of models, connectors, and connections extracted from the SSP (`INFO-006`).
- **Execution Graph:** runnable graph of invocable nodes with the selected executor strategy (`INFO-006`).
