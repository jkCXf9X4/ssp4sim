
## Test Architecture

The suite is split by layer so each test has a clear responsibility:

- `tests/lib/core/`: C++ tests for core runtime primitives such as FMU adapter,
  recorder, ring buffer, signal storage, and start values.
- `tests/lib/utils/`: C++ tests for utility data structures and support code.
- `tests/lib/high_level/`: one C++ smoke test that executes a complete SSP
  through the public simulator entry point.
- `tests/python/high_level/`: Python API tests for full SSP workflows and
  result-file validation.
- `tests/reference_ssp/`: unpacked reference SSP fixtures used by the high-level
  workflow tests.
- `tests/references/` and `tests/resources/`: small static fixtures for focused
  C++ tests.

Keep the C++ high-level layer to a single smoke path through the top-level
modules. Put reference sweeps and result-comparison workflow tests in pytest so
the suite can use Python-side fixture discovery and comparison tools.

## C++ Library Tests

The C++ test binary covers lower-level library behavior and focused integration
checks under `tests/lib/`.

Run it with:

```bash
./build/tests/lib/ssp4sim_tests
```

## High-Level SSP Tests

The high-level reference sweep lives in `tests/python/high_level/` so it can use
pytest parameterization and Python-side CSV/result comparison helpers. Pytest is
configured to collect only `tests/python`.

Run it with:

```bash
pytest -q tests/python
```

The test iterates the unpacked SSP fixtures under
`tests/reference_ssp/build/models/*/ssp`, simulates each co-simulation SSP, and
checks that a complete result CSV is produced. Model-exchange fixtures are
excluded because the current runtime only supports co-simulation FMUs.

The Python tests import `pyssp4sim` from `build/public/python_api` when that
build artifact exists. This prevents high-level tests from accidentally passing
against an unrelated installed wheel.

Known reference runtime failures are marked with strict `xfail` entries in the
test. If an expected-failing SSP starts passing, pytest reports it as an XPASS
failure so the marker must be removed.
