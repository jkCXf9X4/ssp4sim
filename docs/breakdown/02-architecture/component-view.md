---
id: INFO-012
type: info
title: Component view
summary: Internal module decomposition of libssp4sim, graph responsibility split, and the execution pipeline
date: 2026-11-11
status: current
---

# Component view

libssp4sim decomposes into eight bounded module areas with fixed responsibilities and public interfaces.

- **Module areas:**
  - analysis: builds a hierarchical structural view of SSP models, connectors, and connections by recursive traversal; supports FMU components and nested systems.
  - graph: flattens the analysis graph into a flat DAG of invocable FMU nodes; dispatches simulation steps.
  - execution: executor strategies — Jacobi (4 variants), Seidel (2 variants), custom executors.
  - handler: FMU loading, instantiation, stepping, teardown.
  - model: FMU adapter as a graph-invocable model with signal buffers.
  - signal: signal memory layout, recorder handoff, CSV/SQLite sinks.
  - utils: shared helpers — config, allocator, ring buffer, thread pool, tarjan, time, timer.
  - schema_extensions: SSP/FMI schema extensions for parsed XML data.
- **Graph responsibility split:** the analysis graph mirrors the SSP structure without model-to-model edges; the simulation graph adds the model-to-model dependency edges by flattening the analysis graph hierarchy.
- **Execution pipeline:** config selects SSP path, executor method, timing, recording, logging; FmuHandler loads and instantiates FMUs; the analysis graph is built; the graph builder creates executable nodes with recorder hooks; the selected executor advances the graph from start to stop time; models exchange values through signal storage; the recorder copies completed updates and writes outputs.
- **Code map:** module-to-directory layout and headers are implementation (`INFO-016`).
