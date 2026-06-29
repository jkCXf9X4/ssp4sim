# Coverage Targets
<!-- Layer: 04-verification -->
<!-- Stable ID: VER-COVERAGE-001 -->

## Description

This artifact defines verification coverage expectations for SSP4SIM modules. Targets describe desired coverage, not current state.

## Module Coverage

| Module | Current Estimate | Target | Notes |
|---|---|---|---|
| signal/ (storage, recorder) | Good | Core: 80%+ line coverage | SQLite recorder tested; CSV recorder gap noted |
| utils/ (config, ring_buffer, thread_pool) | Good | Core: 80%+ line coverage | Config, node, thread_pool, parallel tested |
| handler/ (FMU adapter) | Moderate | Core: 70%+ line coverage | fmi4c adapter tested; edge cases (error states) untested |
| execution/ (Jacobi, Seidel) | Partial | Core: 60%+ | Tested indirectly through workflow; no strategy-specific unit tests |
| graph/ (analysis, execution graph) | Minimal | Core: 50%+ | Graph builder tested indirectly through workflow |
| model/ (FMU model wrapper) | Minimal | Core: 50%+ | Tested indirectly through workflow |

## What Is Intentionally Not Covered

- CSV recorder: No dedicated unit test — covered by C++ smoke test and Python workflow tests
- Error handling paths: FMU step errors tested only in fmi4c adapter tests; broader error recovery not systematically tested
- Custom executors: Not tested (experimental)
- Specific output format compliance: CSV format correctness not validated beyond existence

## Measurement Approach

- Current: Manual audit from test file inventory
- Target: Automated coverage measurement (gcov or llvm-cov) — not yet implemented
- Coverage changes should be reviewed during PRs but not gated until tooling is in place

## Traceability

- Backward: Traces to test strategy in `product-breakdown/04-verification/test-strategy.md`.
- Sources: Test file inventory, `tests/README.md`, traceability matrix.