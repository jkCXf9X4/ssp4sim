# Tests
<!-- Layer: 04-verification -->


This page describes test layout and test-specific behavior. Build setup lives in
[Build From Source](../docs/build_from_source.md).

## Quick Commands

Build and test commands are documented in [Build From Source](../docs/build_from_source.md).
This page focuses on test layout and behavior; use the commands there when you
need to run the C++ binary or the Python suite.

## Test Architecture

See product-breakdown/04-verification/test-strategy.md for the strategic rationale and test coverage targets.

The suite is split by layer so each test has a clear responsibility:

- `tests/lib/core/`: C++ tests for core runtime primitives such as FMU adapter,
  recorder, ring buffer, signal storage, start values, DuckDB recording, and SQLite recording.
- `tests/lib/utils/`: C++ tests for utility data structures and support code.
- `tests/lib/high_level/`: one C++ smoke test that executes a complete SSP
  through the public simulator entry point with CSV recording enabled.
- `tests/python/high_level/`: Python API tests for full SSP workflows and
  result-file validation.
- `resources/reference_ssp/`: unpacked reference SSP fixtures used by the high-level
  workflow tests.
- `tests/resources/references/` and `tests/resources/`: small static fixtures for focused
  C++ tests.

Keep the C++ high-level layer to a single smoke path through the top-level
modules. Put reference sweeps and result-comparison workflow tests in pytest so
the suite can use Python-side fixture discovery and comparison tools.

## C++ Library Tests

The C++ test binary covers lower-level library behavior and focused integration
checks under `tests/lib/`. Run the binary produced by the build workflow
described in [Build From Source](../docs/build_from_source.md).

`ctest --test-dir build/tests` is currently unreliable. Run the test binary
directly.

Prefer small, focused C++ cases named `test_*.cpp` under `tests/lib/core/` or
`tests/lib/utils/`. Keep `tests/lib/high_level/` to one top-level C++ smoke
path. Put reference sweeps and detailed result comparisons in pytest under
`tests/python/`.

## High-Level SSP Tests

The high-level reference sweep lives in `tests/python/high_level/` so it can use
pytest parameterization and Python-side CSV/result comparison helpers. Pytest is
configured to collect only `tests/python`.

The test iterates the unpacked SSP fixtures under
`resources/reference_ssp/artifacts/models/*/*`, simulates each
co-simulation SSP, and checks that a complete result CSV is produced.
Model-exchange fixtures are excluded because the current runtime only supports
co-simulation FMUs.

The Python tests import `pyssp4sim` from `build/public/python_api` when that
build artifact exists. This prevents high-level tests from accidentally passing
against an unrelated installed wheel.

A separate smoke test exercises the packaged CLI through `venv/bin/pyssp4sim`
against the local `resources/embrace/embrace.json` fixture with temporary
output paths.

See product-breakdown/03-implementation/dependency-policy.md for the fmi4c mode-bit issue and recommended fixture handling.

The reference sweep uses a `0.001` second simulation timestep. The `embrace`
SSP needs this smaller communication step; with a coarse `0.1` second step,
`ECS_HW` returns `fmi2Error` on the first step.

The recorder tests now cover the CSV, DuckDB, and SQLite sinks separately.

The high-level parameter-set coverage is grouped in one parametrized test that
checks the emitted `start_values.csv` for inline system-level parameter sets,
the internal parameter-set fixture in `signal_sine_gain_add`, external `.ssv`
bindings, `.ssv + .ssm` mappings, and representative mapped fixtures with
multiple value types, including the hierarchical `dcmotor` fixture.

All known regression fixtures have been resolved. See product-breakdown/04-verification/regressions.md for history.

After an FMU reaches `fmi2Error` or `fmi2Fatal`, cleanup frees the instance
without calling `fmi2Terminate` so logs keep the original step failure as the
root cause.

## Reference Fixtures

Reference SSP fixtures under `resources/reference_ssp/` may contain a nested Git
repository and generated `build/` content. Do not edit generated outputs unless
the task is explicitly about fixture generation or expected reference data.
When editing this tree, follow `resources/reference_ssp/AGENTS.md`.

Use focused Catch2 tests, focused pytest tests, or one reference SSP simulation
as the smallest validation that proves a change before running broader suites.
