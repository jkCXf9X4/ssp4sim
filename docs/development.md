# Development Guide

This page collects development-oriented workflows for contributors.

## Repository Layout

- [`lib/`](../lib): simulation engine internals
- [`public/`](../public): public app and Python API entry points
- [`tests/`](../tests): Catch2 tests
- [`resources/`](../resources): sample SSPs and input configs
- [`scripts/`](../scripts): helper scripts for result processing/comparison

## Core Development Loop

1. Configure and build using [docs/build_from_source.md](build_from_source.md).
2. Run tests with `./build/tests/ssp4sim_tests`.
3. Run an example scenario:
   - `./build/public/ssp4sim_app/sim_app ./resources/embrace/embrace.json`

## Configuration Work

Simulation runtime configuration reference:

- [docs/configuration.md](configuration.md)

## Profiling And Logging

- Build profiling notes: [docs/profiling.md](profiling.md)
- Logging conventions: [docs/logging_guidlines.md](logging_guidlines.md)

## Release Process

- Linux release pipeline and artifact details: [docs/linux_binary_distribution.md](linux_binary_distribution.md)
- Tagging example:

```bash
git tag v0.1.1
git push origin v0.1.1
```

## Contributing

Welcome to the project!
Open an issue or pull request with:

- As clear as possible problem/solution description
- Tests run
- Before/after output when simulation behavior changes
