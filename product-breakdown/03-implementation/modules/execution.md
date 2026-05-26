# Module: execution
<!-- Layer: 03-implementation -->
<!-- Stable ID: IMP-MOD-EXECUTION-001 -->

## Purpose

Provides executor strategies that advance simulation time and coordinate FMU stepping.

## Key Components

- `ExecutionBase`: Abstract base class for all executors. Manages model vector and invoke interface.
- `Jacobi`: Gauss-Jacobi strategy — all FMUs step concurrently using values from the previous step.
- `Seidel`: Gauss-Seidel strategy — FMUs step sequentially with feedback within a step.
- `ExecutorBuilder`: Creates the appropriate executor based on configuration.
- Custom executors: Additional delay-oriented executor variants.

## Jacobi Variants

- `jacobi_serial`: Sequential Jacobi.
- `jacobi_parallel_fut`: Parallel using std::future.
- `jacobi_parallel_spin`: Parallel using spin-wait synchronization.
- `jacobi_parallel_tbb`: Parallel using TBB task group.

## Seidel Variants

- `seidel_serial`: Sequential Seidel.
- `seidel_parallel`: Parallel Seidel variant.

## Include Boundary

- Path: `lib/include/execution/`
- 10 files including executor, executor_builder, executor_utils, invocable, and variant subdirs.

## Dependencies

- `utils/` — for task thread pool, ring buffer, time conversion
- `model/` — for FmuModel invocation interface

## Notable Patterns

- Strategy pattern: ExecutionBase defines the algorithm interface; Jacobi/Seidel implement specific strategies.
- Parallel variants use different synchronization mechanisms (future, spin-wait, TBB) for experimentation.
- Hot-path logging is compile-time optional.

## Traceability

- Backward: Architecture component view, quality attributes (performance).
- Sources: `lib/include/execution/`, `docs/configuration.md` (executor.method section).