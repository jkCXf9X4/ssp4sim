# Simulation Graph

## Responsibilities

The simulation graph creates the executable runtime from the analysis graph:

1. Creates `FmuModel` instances only for FMU components (skips system models)
2. Allocates `SignalStorage` areas for each model's inputs and outputs
3. Wires connections between models as direct FMU→FMU edges
4. Organizes nodes into a DAG and dispatches simulation steps

## Flattening

The simulation graph flattens the hierarchical analysis graph:

- System models (`ModelKind::system`) are skipped — they have no FMU runtime
- Connections through system boundaries are resolved to direct FMU→FMU edges
- The result is a flat DAG of invocable nodes

## Execution Algorithm

The execution algorithm is constructor-injectable, enabling easy change of algorithm
(Jacobi, Seidel, custom).

Graph → DAG of sub-graphs → DAGs of models