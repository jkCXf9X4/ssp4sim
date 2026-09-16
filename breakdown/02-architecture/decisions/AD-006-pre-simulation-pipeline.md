# AD-006: Pre-Simulation Pipeline — Parsing, Analysis Projection, and Simulation Graph
<!-- Layer: 02-architecture -->
<!-- Status: Accepted -->

> **Status: Supersedes AD-005** — See [AD-006-pre-simulation-pipeline.md](AD-006-pre-simulation-pipeline.md) for the current architecture decision covering the staged pre-simulation pipeline, the analysis handoff boundary, and where graph analysis lives.

## Context

AD-005 defined three layers — Analysis System (pure-data handoff), Analysis Graph Factory (transient graph views), and Simulation Graph — with the simulation layer consuming the analysis system's query API.

Implementation refined this into a staged pre-simulation pipeline. Two refinements diverged from AD-005:

1. **The handoff boundary moved.** The simulation layer does not consume the pure-data system through a query API. Instead, an intermediate pipeline stage projects the pure data into a resolved connection graph, and that resolved graph is the input to simulation-graph construction. The resolved graph is owned by the pipeline stage that produces it and outlives that stage for the duration of construction.

2. **Graph analysis placement changed.** Algebraic-loop detection is no longer performed during analysis. It is performed at execution time over the runtime simulation graph, where the final model-to-model topology (including feedthrough wiring) is known. The analysis stages focus on parsing, structure, and connection resolution.

## Decision

Introduce a staged pre-simulation pipeline under a single orchestration point. Each stage has one responsibility and hands a well-defined artifact to the next. No stage reaches across a boundary into another stage's responsibilities.

### Stage 1 — SSP Parsing (Canonical Data)

- Loads the full SSP into a runtime-free, graph-agnostic canonical model that mirrors the SSP hierarchy.
- Eager loading: the entire topology — systems, components, connectors, connections, parameters, and FMU model descriptions — is resolved before the stage completes. Downstream stages never trigger additional SSP loading.
- The data classes are plain data: vector-based storage, string-based references, no graph-algorithm inheritance.
- Output: the fully-loaded canonical model, which is the single source of truth for all downstream stages.

### Stage 2 — Analysis Projection (Transient Structure + Resolved Topology)

- Projects the canonical model into a transient structural view, then derives the resolved connection topology from it.
- Parameter bindings are applied during projection, so downstream stages consume final values rather than binding metadata.
- Connection resolution is completed here: source and target endpoints are resolved against the full hierarchy, including boundary crossings.
- The projection is built per pipeline invocation; it is not stored on or owned by the canonical model.
- Output: an owned graph of resolved connections that the simulation stage can navigate structurally without string lookups.

### Stage 3 — Simulation Graph Construction (Runtime Models)

- Consumes the resolved connection graph from Stage 2.
- Creates runtime models, allocates signal storage, wires connections (including boundary tracing), and derives execution edges.
- Feedthrough is determined during wiring, using the resolved connection information, not in the analysis stages.
- Output: the executable model set used by the simulation runtime.

### Pipeline Orchestration

- A single entry point composes the stages in order and returns the executable result.
- The canonical model, the projection, and the resolved graph are all pipeline-local; nothing persists beyond construction.
- Debug output is produced alongside the pipeline from the same stage artifacts.

### Graph Analysis at Execution Time

- Algebraic-loop detection (strongly connected components) operates on the runtime simulation graph, where the final model-to-model edges and feedthrough properties exist.
- This keeps analysis-structure concerns (parsing, projection, resolution) separate from execution-order concerns (loop partitioning, scheduling).

## Rationale

1. **Cleaner handoff**: Passing the resolved connection graph to the simulation stage removes the need for the simulation stage to re-derive topology through string-based lookups, and makes the boundary explicit and testable.

2. **Single source of truth**: The canonical model is the only place SSP data lives; projections are derived views that never mutate it. This preserves the eager-loading contract of AD-005.

3. **Algorithm placement matches data availability**: Algebraic-loop detection needs the final feedthrough and wiring state, which only exists after simulation-graph construction. Performing it during analysis would require duplicating that logic or inventing dependencies from analysis onto runtime types.

4. **Staged testability**: Each stage's output is independently verifiable — the canonical model, the resolved graph, and the executable models are each testable in isolation.

## Consequences

- The pre-simulation pipeline supersedes the three-layer split of AD-005. The pure-data canonical model remains the source of truth, but the simulation layer consumes the resolved graph rather than the canonical model directly.
- Analysis types live under the pipeline module tree; legacy analysis/graph paths from earlier decisions are removed.
- Algebraic-loop detection is an execution-time concern over the runtime simulation graph; the analysis stages do not perform SCC.
- Feedthrough is a wiring-time property of the simulation stage, consistent with AD-005.
- Documentation references to the previous handoff boundary (`const AnalysisSystem&` into the simulation layer) are superseded.

## Traceability

- Backward: Supersedes AD-005 (three-layer architecture).
- Sources: `lib/include/pre/` (pipeline module tree), `lib/include/simulation/` (runtime graph analysis).
- Tests: pipeline stage tests under `tests/lib/analysis/`, simulation graph tests under `tests/lib/` (graph/simulation).