# Traceability Matrix
<!-- Layer: 04-verification -->
<!-- Stable ID: VER-TRACE-001 -->

## Description

This artifact maps each product requirement to its verification coverage. Requirements from `product-breakdown/01-product/requirements/` are traced to specific test files and known gaps.

## Matrix

| REQ ID | Description | Test File(s) | Coverage | Notes |
|---|---|---|---|---|
| REQ-001 | SSP 1.0 archive loading | `tests/lib/high_level/` (indirect), `tests/python/high_level/test_reference_ssps.py` | Partial | No dedicated SSP loading unit test; verified through smoke and workflow tests |
| REQ-002 | FMI 2.0 co-simulation lifecycle | `tests/lib/core/test_fmi4c_adapter.cpp`, `tests/lib/high_level/` | Covered | FMU adapter has dedicated unit tests; lifecycle verified through workflow |
| REQ-003 | Configurable execution strategy | `tests/lib/utils/test_parallel.cpp`, `tests/lib/high_level/` | Partial | Executor strategies tested indirectly; no strategy-specific unit tests |
| REQ-004 | CSV output with sample interval | `tests/lib/core/test_data_recorder.cpp`, `tests/lib/high_level/` | Partial | Recorder unit test exists; CSV format not tested in isolation |
| REQ-005 | Local database output with per-storage tables | `tests/lib/core/test_sqlite_recorder.cpp`, `tests/lib/high_level/` | Partial | DuckDB recorder removed per IMP-034; SQLite recorder has unit tests, but the REQ-005 duckdb.enable acceptance criterion is no longer applicable |
| REQ-006 | Local logging with levels and sinks | (none) | Missing | No log-level or log-sink tests exist |
| REQ-007 | Distributed observability with OpenTelemetry | (none) | Missing | No OpenTelemetry export or trace-context tests exist |

## Summary

- **Covered**: 1 of 7 requirements (REQ-002)
- **Partial**: 4 of 7 requirements (REQ-001, REQ-003, REQ-004, REQ-005)
- **Missing**: 2 of 7 requirements (REQ-006, REQ-007)

## Traceability

- Backward: Traces to requirements in `product-breakdown/01-product/requirements/`.
- Sources: Test file inventory, `tests/README.md`.
