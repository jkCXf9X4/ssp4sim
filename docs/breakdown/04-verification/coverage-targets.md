---
id: INFO-029
type: info
title: Coverage Targets
summary: Per-module line-coverage targets and what is intentionally not covered; targets are goals, not current state
date: 2026-11-11
status: current
---

# Coverage Targets

Targets describe desired coverage, not current state; measurement is manual today.

- **signal/** (storage, recorder): current good, target 80%+ line; SQLite recorder tested; CSV recorder gap noted.
- **utils/** (config, ring_buffer, thread_pool): current good, target 80%+; config, node, thread_pool, parallel tested.
- **handler/** (FMU adapter): current moderate, target 70%+; fmi4c adapter tested; error-state edge cases untested.
- **execution/** (Jacobi, Seidel): current partial, target 60%+; tested only indirectly through workflow; no strategy-specific unit tests.
- **graph/** and **model/**: current minimal, target 50%+; tested only indirectly through workflow.
- **Intentionally not covered:**
  - CSV recorder has no dedicated unit test; covered by the C++ smoke test and Python workflow tests.
  - FMU step-error paths tested only in the fmi4c adapter tests; broader error recovery not systematically tested.
  - Custom executors are experimental and untested.
  - CSV format correctness not validated beyond existence.
- **Measurement:** current coverage is a manual audit from the test-file inventory; automated measurement (gcov or llvm-cov) is not yet implemented; coverage is reviewed in PRs but not gated until tooling lands.
