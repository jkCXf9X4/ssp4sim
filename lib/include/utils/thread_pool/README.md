# Thread Pool Tradeoff Documentation

## Status / Context

This document satisfies **IMP-028** (Restructure Thread Pools with Tradeoff Documentation). It describes the two thread pool implementations available in `lib/include/utils/`, their design differences, and when to choose each.

## Overview

Two thread pool implementations coexist in the utility layer, each targeting a distinct performance profile and use case:

1. **`ThreadPool`** — defined in `utils/task_thread_pool.hpp` / `task_thread_pool.cpp`. A generic producer-consumer pool that uses `std::counting_semaphore` (C++20) for work notification and returns `std::future<return_type>` to callers. Suitable for mixed work types where each task produces a distinct result.

2. **`ThreadPool2`** — defined in `utils/task_thread_pool2.hpp` / `task_thread_pool2.cpp`. A specialized pool designed for the fixed `task_info{Invocable*, StepData}` task type. It uses epoch-based `std::condition_variable` signaling combined with spin-wait on `std::atomic<bool>` done flags, and a LIFO stack for task ordering. Low-latency path for tight executor loops.

## Decision Matrix

| Attribute | ThreadPool (Semaphore) | ThreadPool2 (Spin/Epoch) |
|-----------|------------------------|---------------------------|
| **Synchronization** | `std::counting_semaphore` (C++20) | Epoch-based `std::condition_variable` + spin on `dones[]` |
| **Task type** | Generic `std::function<void()>` | Fixed `task_info{Invocable*, StepData}` |
| **Return value** | `std::future<return_type>` | `std::atomic<bool>` done flags |
| **Task ordering** | FIFO queue | LIFO stack (cache affinity) |
| **Bulk launch** | Enqueue one-by-one | `ready(nodes)` + batch enqueue |
| **Completion detection** | `future::get()` per task | Spin on `dones[]` array |
| **CPU overhead (idle)** | Low (blocked on semaphore) | Low (blocked on condition_variable) |
| **CPU overhead (busy)** | Medium (future creation) | Low (raw flag polling) |
| **Latency** | Medium (future + packaged_task) | Low (direct flag write) |
| **Use case** | Generic async tasks, mixed work types | Fixed invocation pattern, executor loops |
| **Current consumers** | `JacobiParallelFutures` | `JacobiParallelSpin` |
| **Proposed for** | IMP-021 (ParallelSeidel) | IMP-025 (Hybrid SCC iteration) |

## Selection Guide

**Choose `ThreadPool` (semaphore) when:**

- Tasks have heterogeneous signatures — the pool accepts any `std::function<void()>`.
- Each task produces a value that must be retrieved via `std::future` at a later point.
- The workload mixes long-running and short tasks; FIFO ordering is acceptable.
- The caller needs fine-grained per-task completion tracking rather than bulk completion.
- Example: a scheduler that dispatches independent model evaluations and collects results as futures.

**Choose `ThreadPool2` (spin/epoch) when:**

- All tasks share the same `task_info{Invocable*, StepData}` structure.
- The execution pattern is a tight loop: set up work, notify all workers in bulk, wait for all to complete, repeat.
- Latency from task submission to flag write matters — the LIFO stack keeps recently-used data in cache.
- Bulk notification (`ready(nodes)` → batch `enqueue`) maps naturally onto the execution model.
- Example: an executor that repeatedly dispatches the same set of nodes across SCC iterations and needs low-latency completion detection.

**Do not use `ThreadPool2` for** general-purpose async dispatch, tasks with different return types, or workloads where tasks are not amenable to batch launch.

## Build Linkage

Both pools are compiled directly into the `ssp4sim_lib` target. The `lib/CMakeLists.txt` file uses:

```cmake
file(GLOB_RECURSE ALL_SRC CONFIGURE_DEPENDS "*.hpp" "*.cpp")
```

This picks up every `.hpp` and `.cpp` file under `lib/`, including both pool implementations. There is **no separate CMake target** for either pool.

**Header paths** (from any consumer in the project):

```cpp
#include "utils/task_thread_pool.hpp"   // ThreadPool
#include "utils/task_thread_pool2.hpp"  // ThreadPool2
```

**To add a new consumer**: include the desired header and construct the pool object. No CMake changes are needed as long as the consumer is already linked against `ssp4sim_lib`. If the consumer lives in a separate target, add `target_link_libraries(consumer_target PRIVATE ssp4sim_lib)`.

## Traceability

- **Satisfies**: IMP-028
- **Enables**: IMP-021 (ParallelSeidel), IMP-025 (Seidel-Jacobi hybrid)
- **Layer**: 03-implementation (utility infrastructure), 02-architecture (concurrency architecture)