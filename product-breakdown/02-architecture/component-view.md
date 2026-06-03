# Component View
<!-- Layer: 02-architecture -->
<!-- Stable ID: ARCH-COMPONENT-001 -->

## Description

This artifact describes the internal component structure of libssp4sim, the core simulation engine.

## Module Areas

| Module | Responsibility | Public Interface |
|---|---|---|
| **analysis/** | Builds a hierarchical structural view of SSP models, connectors, and connections via recursive traversal. Supports both FMU Components and nested Systems. | AnalysisGraph, AnalysisGraphBuilder, AnalysisModel (fmu/system), AnalysisConnector, AnalysisConnection |
| **execution/** | Executor strategies — Jacobi (4 variants), Seidel (2 variants), custom executors | ExecutionBase, Jacobi, Seidel, ExecutorBuilder |
| **graph/** | Flattens the hierarchical analysis graph into a flat DAG of invocable FMU nodes. Dispatches simulation steps to the executor. | Graph, GraphBuilder, InvocableNode, AsyncNode |
| **handler/** | FMU loading, instantiation, stepping, teardown | FmuHandler, FmuInfo, FmuInstance, CoSimulationModel |
| **model/** | FMU adapter as graph-invocable model with signal buffers | FmuModel, ModelConnection, ModelConnector |
| **signal/** | Signal memory layout, recorder handoff, CSV/SQLite sinks | SignalStorage, DataRecorder, RecorderSink |
| **utils/** | Shared helpers — config, allocator, ring buffer, thread pool, tarjan, time, timer | Config, RingBuffer, TaskThreadPool |
| **schema_extensions/** | SSP/FMI schema extensions for parsed XML data | SSP_Ext, FMI2 modelDescription_Ext |

### Graph Responsibility Separation (AD-003)

The analysis graph mirrors the SSP structure (models, connectors, connections, internal variables) 
without model-to-model edges. The simulation graph creates the model-to-model dependency graph 
by flattening the analysis graph hierarchy. See AD-003 for details.

## Key Internal Dependencies

```
Simulator → Simulation
Simulation → FmuHandler, GraphBuilder, AnalysisGraphBuilder, Graph, DataRecorder
FmuHandler → FmuInfo → FmuModel
GraphBuilder → Graph
Graph → AsyncNode, ExecutionBase, InvocableNode
AnalysisGraphBuilder → AnalysisGraph
FmuModel → SignalStorage
DataRecorder → SignalStorage, RecorderSink
ExecutionBase → Jacobi, Seidel
```

## Execution Pipeline

1. Configuration selects SSP path, executor method, timing, recording, logging
2. `FmuHandler` loads and instantiates FMUs
3. `AnalysisGraphBuilder` builds structural view from SSP
4. `GraphBuilder` creates executable nodes with recorder hooks
5. Selected executor (`Jacobi` or `Seidel`) advances graph from start to stop time
6. Models exchange values through `SignalStorage`
7. `DataRecorder` copies completed updates and writes configured outputs

## Traceability

- Backward: Traces to context view in `product-breakdown/02-architecture/context-view.md`.
- Sources: `lib/CMakeLists.txt`, `lib/include/readme.md`, `lib/include/signal/readme.md`, `lib/class_diagram.md`.
