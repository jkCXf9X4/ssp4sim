# Traceability Matrix
<!-- Layer: 04-verification -->
<!-- Stable ID: VER-TRACE-001 -->

## Description

This artifact maps each product requirement to its verification coverage. Requirements from `breakdown/01-product/requirements/` are traced to specific test files and known gaps.

## Matrix

| REQ ID | Description | Test File(s) | Coverage | Notes |
|---|---|---|---|---|
| REQ-001 | SSP 1.0 archive loading | `tests/lib/high_level/` (indirect), `tests/python/high_level/test_reference_ssps.py` | Partial | No dedicated SSP loading unit test; verified through smoke and workflow tests |
| REQ-002 | FMI 2.0 co-simulation lifecycle | `tests/lib/core/test_fmi4c_adapter.cpp`, `tests/lib/high_level/` | Covered | FMU adapter has dedicated unit tests; lifecycle verified through workflow |
| REQ-003 | Configurable execution strategy | `tests/lib/utils/test_parallel.cpp`, `tests/lib/high_level/` | Partial | Executor strategies tested indirectly; no strategy-specific unit tests |
| REQ-004 | CSV output with sample interval | `tests/lib/core/test_data_recorder.cpp`, `tests/lib/high_level/` | Partial | Recorder unit test exists; CSV format not tested in isolation |
| REQ-005 | Local database output with per-storage tables | `tests/lib/core/test_sqlite_recorder.cpp`, `tests/lib/high_level/` | Partial | DuckDB recorder removed per IMP-034; REQ-005 acceptance is all SQLite (per-simulation WAL files, optional shared-file mode); SQLite recorder has unit tests in tests/lib/core/test_sqlite_recorder.cpp; SQLite WAL concurrency covered in test_sqlite_recorder.cpp |
| REQ-006 | Local logging with levels and sinks | (none) | Partial | Logging IS implemented in code (ssp4cpp::utils::log used in lib/include/simulation.cpp, lib/public_include/simulator.cpp, lib/include/pre/3_simulation_graph/elements/model_connector.hpp, model_connection.hpp, model_fmu.cpp); dedicated log tests missing — TEST gap, not code gap |
| REQ-007 | Distributed observability with OpenTelemetry | (none) | Missing | Feature NOT implemented in code — PRODUCT gap (no opentelemetry references in lib/; verified grep -rln opentelemetry lib/ = 0) |

**Untraced**: REQ-009..REQ-015 exist in `breakdown/01-product/requirements/` (REQ-008 does not exist) but are not yet traced in this matrix.

## Summary

- **Covered**: 1 of 14 requirements (REQ-002)
- **Partial**: 5 of 14 requirements (REQ-001, REQ-003, REQ-004, REQ-005, REQ-006)
- **Missing**: 1 of 14 requirements (REQ-007)
- **Untraced**: 7 of 14 requirements (REQ-009..REQ-015)

## Traceability

- Backward: Traces to requirements in `breakdown/01-product/requirements/`.
- Sources: Test file inventory, `tests/README.md`.
