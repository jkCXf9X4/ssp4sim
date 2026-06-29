# Module: utils
<!-- Layer: 03-implementation -->
<!-- Stable ID: IMP-MOD-UTILS-001 -->

## Purpose

Shared helpers used across all other modules. Provides low-level infrastructure for configuration, memory management, time conversion, graph algorithms, and thread management.

## Key Components

- `Config`: JSON configuration file loading and dotted-key access. Supports required (get*) and optional (getOr) reads with [TIME] substitution.
- `RingBuffer`: Fixed-capacity ring buffer backing SignalStorage data areas.
- `TaskThreadPool`: Thread pool for parallel execution tasks. Two variants exist.
- `Tarjan`: Tarjan's algorithm for finding strongly connected components in the analysis graph.
- `Allocator`: Custom memory allocator utilities.
- `Time`: Time conversion and formatting helpers.
- `Timer`: High-resolution timing utilities.
- `Node`/`NodeIterator`: Graph node/edge iteration utilities.
- `Map`/`Vector`: Additional container utilities.

## Include Boundary

- Path: `lib/include/utils/`
- 26 files covering all utility types.

## Dependencies

- External: nlohmann_json (for config parsing)
- No internal module dependencies (utils is the lowest-level module)

## Notable Patterns

- Utils is the foundation module with zero internal dependencies.
- Config supports array index access (`some_array.0`) and TIME substitution in string values.
- Thread pool has two implementations — the newer variant may replace the older one.
- RingBuffer capacity is fixed at allocation time.

## Traceability

- Backward: Architecture component view (utils/ module area).
- Sources: `lib/include/utils/`, `docs/configuration.md`.