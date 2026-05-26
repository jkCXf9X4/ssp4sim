# Domain Model
<!-- Layer: 01-product -->
<!-- Stable ID: PRO-DOMAIN-001 -->

## Description

This artifact describes the core domain concepts and their relationships. It provides a shared vocabulary for understanding the problem domain.

## Core Concepts

### SSP Archive
A structured archive (or unpacked directory) conforming to the SSP 1.0 standard. Contains system structure description (SSD), parameter values (SSV/SSM), and referenced FMU binaries.

### FMU (Functional Mock-up Unit)
A packaged co-simulation model conforming to FMI 2.0. Contains the model implementation (shared library), model description XML, and optional resources.

### Execution Strategy
The algorithm used to advance simulation time and coordinate FMU stepping:
- **Gauss-Jacobi**: Models step concurrently using values from the previous step.
- **Gauss-Seidel**: Models step sequentially, feeding updated values forward within the same step.

### Recording
The pipeline that captures simulation results. Supports CSV (default) and DuckDB (alternative) output sinks.

### Signal Storage
In-memory buffers that hold FMU input and output values during simulation. The recorder copies completed updates into recorder-owned buffers for output.

### Simulation Graph
A directed acyclic graph (DAG) of invocable nodes that represents the execution order of models. Built from analysis results and recorder hooks.

## Relationships

```
SSP Archive → contains → FMU(s)
FMU → has → Model Description (FMI 2.0)
Simulation → uses → Execution Strategy
Simulation → creates → Recording → CSV or DuckDB
Simulation → builds → Analysis Graph → Execution Graph
Execution Graph → schedules → FMU Model → reads/writes → Signal Storage
```

## Traceability

- Backward: Traces to capabilities in `product-breakdown/01-product/capabilities.md`.
- Sources: `lib/include/readme.md`, `lib/class_diagram.md`, `lib/class_description.md`.