# Tests

This page describes test layout and test-specific behavior. Build setup lives in
[Build From Source](../docs/build_from_source.md).

## Quick Commands

Configure with the tests enabled before running the C++ test binary:

```bash
cmake --preset=vcpkg -DSSP4SIM_BUILD_TEST=ON
cmake --build build
./build/tests/lib/ssp4sim_tests
```

Python tests require the Python API build artifact under
`build/public/python_api`:

```bash
cmake --preset=vcpkg -DCMAKE_BUILD_TYPE=Release -DSSP4SIM_BUILD_PYTHON_API=ON
cmake --build build
pytest -q tests/python
```

## Test Architecture

The suite is split by layer so each test has a clear responsibility:

- `tests/lib/core/`: C++ tests for core runtime primitives such as FMU adapter,
  recorder, Influx recorder sink, ring buffer, signal storage, and start values.
- `tests/lib/utils/`: C++ tests for utility data structures and support code.
- `tests/lib/high_level/`: one C++ smoke test that executes a complete SSP
  through the public simulator entry point with CSV recording enabled.
- `tests/python/high_level/`: Python API tests for full SSP workflows and
  result-file validation.
- `tests/resources/reference_ssp/`: unpacked reference SSP fixtures used by the high-level
  workflow tests.
- `tests/resources/references/` and `tests/resources/`: small static fixtures for focused
  C++ tests.

Keep the C++ high-level layer to a single smoke path through the top-level
modules. Put reference sweeps and result-comparison workflow tests in pytest so
the suite can use Python-side fixture discovery and comparison tools.

## C++ Library Tests

The C++ test binary covers lower-level library behavior and focused integration
checks under `tests/lib/`. Run it with `./build/tests/lib/ssp4sim_tests`.

`ctest --test-dir build/tests` is currently unreliable. Run the test binary
directly.

Prefer small, focused C++ cases named `test_*.cpp` under `tests/lib/core/` or
`tests/lib/utils/`. Keep `tests/lib/high_level/` to one top-level C++ smoke
path. Put reference sweeps and detailed result comparisons in pytest under
`tests/python/`.

The Influx recorder tests use a fake in-process writer for offline coverage.
One live integration case also targets the local InfluxDB 3 service when
`SSP4SIM_INFLUX_TOKEN` or `~/.influxdb/docker/explorer/config/config.json`
provides a token. If neither is available, the live case skips cleanly.

## High-Level SSP Tests

The high-level reference sweep lives in `tests/python/high_level/` so it can use
pytest parameterization and Python-side CSV/result comparison helpers. Pytest is
configured to collect only `tests/python`.

The test iterates the unpacked SSP fixtures under
`tests/resources/reference_ssp/build/models/*/ssp`, simulates each co-simulation SSP, and
checks that a complete result CSV is produced. Model-exchange fixtures are
excluded because the current runtime only supports co-simulation FMUs.

The Python tests import `pyssp4sim` from `build/public/python_api` when that
build artifact exists. This prevents high-level tests from accidentally passing
against an unrelated installed wheel.

A separate smoke test exercises the packaged CLI through `venv/bin/pyssp4sim`
against the local `resources/embrace/embrace.json` fixture with temporary
output paths.

The fmi4c loader marks Linux shared libraries executable before `dlopen`, so
loading tracked fixtures directly could dirty Git by changing `.so` file mode
bits. Prefer tests that copy mutable fixtures into a temporary directory before
loading them.

The reference sweep uses a `0.001` second simulation timestep. The `embrace`
SSP needs this smaller communication step; with a coarse `0.1` second step,
`ECS_HW` returns `fmi2Error` on the first step.

Two dedicated high-level tests also check that the emitted `start_values.csv`
captures the applied values for a system-level inline parameter set fixture and
an external `.ssv` parameter set fixture.

Known reference runtime failures are marked with strict `xfail` entries in the
test. If an expected-failing SSP starts passing, pytest reports it as an XPASS
failure so the marker must be removed. The current expected failures are
`dcmotor`, because the runtime does not yet support hierarchical SSP systems

After an FMU reaches `fmi2Error` or `fmi2Fatal`, cleanup frees the instance
without calling `fmi2Terminate` so logs keep the original step failure as the
root cause.

## Reference Fixtures

Reference SSP fixtures under `tests/resources/reference_ssp/` may contain a nested Git
repository and generated `build/` content. Do not edit generated outputs unless
the task is explicitly about fixture generation or expected reference data.
When editing this tree, follow `tests/resources/reference_ssp/AGENTS.md`.

Use focused Catch2 tests, focused pytest tests, or one reference SSP simulation
as the smallest validation that proves a change before running broader suites.
