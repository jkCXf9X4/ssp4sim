# Simulation Graph

## Responsibilities

The simulation graph creates the executable runtime from the analysis graph:

1. Creates `FmuModel` instances only for FMU components (skips system models)
2. Allocates `SignalStorage` areas for each model's inputs and outputs
3. Wires connections between models as direct FMU→FMU edges
4. Organizes nodes into a DAG and dispatches simulation steps
