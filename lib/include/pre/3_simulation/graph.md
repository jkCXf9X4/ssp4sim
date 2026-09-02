# Simulation Graph

## Responsibilities

The simulation graph creates the executable runtime from the analysis graph:

1. Creates `FmuModel` instances only for FMU components (skips system models)
2. Allocates `SignalStorage` areas for each model's inputs and outputs
3. Wires connections between models as direct FMU→FMU edges
4. Organizes nodes into a DAG and dispatches simulation steps

## Data Access Modes

`wire_connections` precomputes, per connection, how the target samples its
source (`ConnectionInfo::mode`):

- `StartTime` — sample the source at the start of the target model's step span
  (chosen for feedthrough / `delay == 0` edges)
- `EndTime` — sample the source at the end of the target model's step span
  (chosen for delayed / temporal edges)
- `Latest` — zero-order hold on the newest data at/ before the requested input
  time (default)

A mutable `ConnectionInfo::time_offset` (int64, can be negative) shifts the
reference time for every mode, so an executor/scheduler algorithm can tune when
a connection samples its source. Derivative forwarding is orthogonal and uses
whichever source area is selected.
