---
id: TR-001
type: trace
title: Traceability Matrix
summary: Requirement-to-test coverage map — one covered, five partial, one missing, seven untraced
date: 2026-11-11
status: current
pb_exempt: true
---

# Traceability Matrix

Maps each product requirement to its verification coverage: test locations and known gaps.

**Coverage map**

| REQ ID | Description | Test File(s) | Coverage | Notes |
|---|---|---|---|---|
| REQ-001 | SSP 1.0 archive loading | `tests/lib/high_level/` (indirect), `tests/python/high_level/test_reference_ssps.py` | Partial | No dedicated SSP-loading unit test; verified through smoke and workflow tests |
| REQ-002 | FMI 2.0 co-simulation lifecycle | `tests/lib/core/test_fmi4c_adapter.cpp`, `tests/lib/high_level/` | Covered | FMU adapter has dedicated unit tests; lifecycle verified through workflow |
| REQ-003 | Configurable execution strategy | `tests/lib/utils/test_parallel.cpp`, `tests/lib/high_level/` | Partial | Executor strategies tested indirectly; no strategy-specific unit tests |
| REQ-004 | CSV output with sample interval | `tests/lib/core/test_data_recorder.cpp`, `tests/lib/high_level/` | Partial | Recorder unit test exists; CSV format not tested in isolation |
| REQ-005 | Local database output with per-storage tables | `tests/lib/core/test_sqlite_recorder.cpp`, `tests/lib/high_level/` | Partial | DuckDB recorder removed; acceptance is all SQLite (per-simulation WAL files, optional shared-file mode); WAL concurrency covered in the recorder unit tests |
| REQ-006 | Local logging with levels and sinks | (none) | Partial | Logging implemented (ssp4cpp::utils::log); dedicated log tests missing — TEST gap, not code gap |
| REQ-007 | Distributed observability with OpenTelemetry | (none) | Missing | Feature not implemented in code — PRODUCT gap (no opentelemetry references in lib/) |

**Untraced:** REQ-009 through REQ-015 exist in the requirements folder but have no matrix rows yet.

**Summary:**
- **Covered:** 1 of 14 requirements (REQ-002).
- **Partial:** 5 of 14 requirements (REQ-001, REQ-003, REQ-004, REQ-005, REQ-006).
- **Missing:** 1 of 14 requirements (REQ-007).
- **Untraced:** 7 of 14 requirements (REQ-009 through REQ-015).
