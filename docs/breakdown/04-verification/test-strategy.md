---
id: INFO-030
type: info
title: Test Strategy
summary: Three-layer test pyramid — C++ unit tests, one C++ smoke test, Python workflow tests — and the layer-boundary rationale
date: 2026-11-11
status: current
---

# Test Strategy

SSP4SIM tests form a three-layer pyramid; the concrete layout and run commands live in the tests readme.

- **Pyramid:**
  - C++ unit tests cover runtime primitives.
  - One C++ integration smoke test drives the public simulator entry point.
  - Python workflow tests cover reference-SSP sweeps and result validation.
- **C++ boundary:** C++ tests verify low-level runtime correctness where Python overhead is undesirable or direct library API access is needed.
- **Python boundary:** Python tests handle reference sweeps, result validation, and multi-fixture orchestration where pytest tooling (parameterization, fixtures, xfail markers) adds value.
- **Smoke constraint:** the C++ high-level layer stays one smoke path to avoid duplicating Python workflow logic in C++.
- **Coverage state:**
  - SQLite recorder has dedicated C++ tests.
  - CSV recorder is tested only through the C++ smoke test and Python workflow tests.
  - FMU adapter and model lifecycle are tested through C++ core tests and indirectly through Python workflows.
  - Reference fixtures in `resources/reference_ssp/` are the primary workflow test data.
- **Known limits:** model-exchange SSP fixtures are excluded (co-simulation only); the fmi4c mode-bit issue and recommended fixture handling are in the implementation dependency policy (`INFO-018`).
