# Glossary
<!-- Layer: 01-product -->
<!-- Stable ID: PRO-GLOSSARY-001 -->

## Description

Standardized terms and definitions used across the SSP4SIM project. Maintains consistency in communication.

## Terms

| Term | Definition |
|---|---|
| **SSP** | System Structure and Parameterization — a standard for packaging co-simulation setups. |
| **SSD** | System Structure Description — the XML file describing model topology and connections. |
| **SSV** | System Structure Parameter Values — file with parameter value overrides. |
| **SSM** | System Structure Model — file mapping parameters to FMU variables. |
| **FMU** | Functional Mock-up Unit — a packaged model conforming to the FMI standard. |
| **FMI 2.0** | Functional Mock-up Interface version 2.0 — standard for model exchange and co-simulation. |
| **Co-Simulation** | FMI mode where each FMU is simulated independently and coupled through input/output connections. |
| **Gauss-Jacobi** | Execution strategy where all FMUs step concurrently using values from the previous time step. |
| **Gauss-Seidel** | Execution strategy where FMUs step sequentially, allowing updated values to propagate within a step. |
| **SignalStorage** | In-memory buffer holding typed signal values for one simulation area. |
| **DataRecorder** | Component that copies completed signal updates and writes them to configured output sinks. |
| **AnalysisGraph** | Structural view of models, connectors, and connections extracted from the SSP. |
| **Execution Graph** | Runnable graph of invocable nodes with the selected executor strategy. |

## Traceability

- Backward: Traces to domain model in `product-breakdown/01-product/domain-model.md`.
- Sources: `lib/include/readme.md`, `lib/class_description.md`, SSP 1.0 standard.