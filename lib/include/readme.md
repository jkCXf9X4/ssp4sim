# Library Internals Overview

The headers under `lib/include/` implement the simulation engine behind the
public CLI and Python API. This page is a short orientation guide; use the
module-level READMEs and source files for details.

## Top-Level Lifecycle

`Simulator` owns resources that span a simulation run, including the loaded SSP
archive, configuration, and the active `Simulation` instance.

`Simulation` owns the runtime components for one execution:

- `handler::FmuHandler` loads and prepares FMUs.
- `analysis::graph::AnalysisGraphBuilder` analyzes SSP models, connectors, and
  connections.
- `graph::GraphBuilder` turns the analysis graph into executable graph nodes.
- `graph::Graph` invokes nodes through the selected executor.
- `signal::DataRecorder` records signal data when recording is enabled.

## Core Areas

- `analysis/`: builds the model and connection graph from SSP structures before
  execution.
- `execution/`: contains executor strategies such as Jacobi, Seidel, and
  custom delay-oriented executors.
- `graph/`: owns invocable nodes and dispatches simulation steps to the active
  executor.
- `handler/`: wraps FMU loading, setup, stepping, teardown, and FMI adapter
  behavior.
- `model/`: adapts FMUs into graph-invocable simulation models with input and
  output signal storage.
- `signal/`: owns signal memory layout and recorder handoff. See
  [`signal/readme.md`](signal/readme.md).
- `utils/`: shared helpers such as ring buffers, time conversion, and
  configuration access.

## Data Flow

1. Configuration selects the SSP path, execution method, timing, recording, and
   logging options.
2. FMUs are loaded and instantiated through `handler/`.
3. Analysis builds a structural view of models and connections.
4. The graph builder creates executable model nodes and attaches recorder hooks.
5. The selected executor advances the graph from start time to stop time.
6. Models exchange values through `signal::SignalStorage`.
7. The recorder copies completed signal updates and writes configured outputs.
