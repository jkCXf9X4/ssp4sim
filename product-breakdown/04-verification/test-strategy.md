# Test Strategy
<!-- Layer: 04-verification -->
<!-- Stable ID: VER-STRATEGY-001 -->

## Description

This artifact formalizes the SSP4SIM testing approach — what is tested at each level, why, and how the test layers relate.

## Test Pyramid

The test suite follows a three-layer pyramid:

1. **Unit Tests (C++)**: Test individual runtime primitives in isolation. Catch2-based. Targets module-level correctness and edge cases.

2. **Integration Smoke Test (C++)**: One C++ smoke test that exercises a complete SSP through the public simulator entry point. Verifies end-to-end execution without workflow-level tooling.

3. **Workflow Tests (Python)**: Reference SSP sweep using pytest parameterization. Targets output validation, fixture discovery, and known failure tracking.

See tests/README.md for the concrete test layout and run commands.

## Layer Boundary Rationale

- C++ tests verify low-level correctness of runtime primitives where Python overhead is undesirable or where direct library API access is needed.
- Python tests handle reference sweeps, result validation, and multi-fixture orchestration where pytest's tooling (parameterization, fixture management, xfail markers) adds value.
- The C++ high-level layer is intentionally kept to one smoke path to avoid duplicating Python workflow logic in C++.

## Coverage Notes

- DuckDB recorder has dedicated C++ tests.
- CSV recorder is tested only through the C++ high-level smoke test and Python workflow tests (no dedicated unit test).
- FMU adapter and model lifecycle are tested through C++ core tests and indirectly through Python workflow tests.
- Reference fixtures in `tests/resources/reference_ssp/` are the primary workflow test data.

## Known Limitations

- See tests/README.md for test-running caveats.
- See 03-implementation/dependency-policy.md for the fmi4c mode-bit issue and recommended fixture handling.
- Model-exchange SSP fixtures are excluded (only co-simulation is supported).

## Traceability

- Backward: Traces to requirements in `product-breakdown/01-product/requirements/`.
- Sources: `tests/README.md`, `tests/resources/reference_ssp/AGENTS.md`.
