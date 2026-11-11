---
id: INFO-023
type: info
title: execution module
summary: Executor strategies that advance simulation time and coordinate FMU stepping
date: 2026-11-11
status: current
---

# execution module

Provides executor strategies that advance simulation time and coordinate FMU stepping, selected by configuration.

- **Role:** `ExecutionBase` defines the executor interface; `ExecutorBuilder` creates the configured executor; strategy pattern, one concrete strategy per algorithm.
- **Jacobi variants:** `jacobi_serial`, `jacobi_parallel_fut` (std::future), `jacobi_parallel_spin` (spin-wait), `jacobi_parallel_tbb` (TBB task group); all FMUs step concurrently on previous-step values.
- **Seidel variants:** `seidel_serial`; `seidel_parallel` is not implemented — selecting it throws at config selection.
- **la2 (loop-aware):** factory-assembled stack, not a class — SCC detection, one `LinearSubstepExecutor` or `GeometricSubstepExecutor` per loop SCC, the condensed component DAG run by an outer `SerialSeidel`, and a mixed-policy `La2DataAccessResolver` (cross-SCC Latest, intra-SCC StartTime). Outer `ParallelSeidel` (`la2.parallel`) is not implemented and throws during stack assembly.
- **Sub-step executors:** `LinearSubstepExecutor` splits the macro step into equal sub-steps (`iterations`); `GeometricSubstepExecutor` shrinks sub-steps, each covering `factor` of the remaining time until the remainder is at or below `threshold`, then takes the rest of the macro step whole.
- **Include boundary:** `lib/include/simulation/executor/` — `executor_base.hpp`/`.cpp`, `executor_utils.hpp`, variant subdirs `jacobi/`, `seidel/`, `loop_aware/`, `substep/`, `custom/`; `ExecutorBuilder` sits one level up at `lib/include/simulation/executor_builder.hpp`.
- **Dependencies:** `utils/` (task thread pool, ring buffer, time conversion) and `model/` (FmuModel invocation interface).
