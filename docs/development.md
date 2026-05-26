# Development Guide
<!-- Layer: 03-implementation, 02-architecture, 05-operation -->


This page collects contributor conventions. Build commands live in
[Build From Source](build_from_source.md), test-suite details live in
[tests/README.md](../tests/README.md), and runtime configuration lives in
[Configuration](configuration.md).

## Repository Layout

- [`lib/`](../lib): simulation engine internals
- [`public/`](../public): public app and Python API entry points
- [`tests/lib/`](../tests/lib): Catch2-based C++ unit and integration tests
  split by layer (`core/`, `utils/`, and one `high_level/` smoke test)
- [`tests/python/`](../tests/python): pytest-based high-level workflow tests
- [`tests/resources/reference_ssp/`](../tests/resources/reference_ssp): unpacked reference fixture
  repository used by high-level workflow tests
- [`resources/`](../resources): SSP/SSD/SSM/SSV examples, reference inputs,
  and sample scenarios
- [`scripts/`](../scripts): helper scripts for result processing/comparison
- [`3rdParty/`](../3rdParty): vendored dependencies such as `ssp4cpp` and
  `fmi4c`

For detailed architecture information, see the [architecture layer](../product-breakdown/02-architecture/).

## Core Development Loop

1. Build with the smallest relevant source-build workflow.
2. Run the focused test that proves the change.
3. Run one example simulation when behavior changes user-visible runtime output.
4. Update the relevant documentation when behavior, commands, architecture, or
   workflow assumptions change.

Use [Build From Source](build_from_source.md) for exact build commands and
[tests/README.md](../tests/README.md) for test selection.

## Coding Style

- C++ formatting follows a 4-space indent with braces on the next line.
- Class names use `PascalCase`; functions and files generally use
  `snake_case`.
- Keep includes ordered and local headers grouped consistently with nearby
  files under `lib/include/**`.
- No enforced formatter is checked in; match surrounding style closely.
- Keep changes direct and explicit. Minimize duplication, but do not introduce
  abstractions that hide important simulation or memory-layout details.
- Prefer repository-owned code in `lib/`, `public/`, `tests/`, `resources/`,
  and `scripts/` over patching vendored dependencies in `3rdParty/`, unless
  the task is explicitly about vendored behavior.
- Treat this as active experimental software: clear root-cause fixes are
  preferred over compatibility shims and broad defensive workarounds.

## Dependencies And Generated Data

- CMake presets (`CMakePresets.json`) expect vcpkg; see
  [vcpkg.md](../vcpkg.md) for setup.
- Common optional build flags: `SSP4SIM_BUILD_TEST`,
  `SSP4SIM_BUILD_PYTHON_API`, and `SSP4SIM_LOG_HOT_PATH`.
- Use the repo-local `venv` for Python commands when it exists. Prefer
  `. venv/bin/activate && <command>` or `venv/bin/python <command>` over the
  system Python for workflow scripts and pytest.
- Do not casually rewrite dependency setup. Python dependencies, vcpkg
  features, and platform-specific FMU tooling are part of the reproducible
  environment.
- Many FMU and SSP workflows assume Linux `x86_64` binaries. Preserve
  executable permissions for libraries under `binaries/`, and prefer copying
  fixtures to a temporary location when a loader or unpacking step may mutate
  files.
- Treat `build/`, unpacked archives, result CSVs, logs, and FMU/SSP binaries
  as generated or fixture data. Do not normalize, repackage, or rewrite them
  unless the requested change explicitly requires it.

## Configuration Work

Update the configuration reference when adding, removing, renaming, or changing
defaults for keys read from `utils::Config`.

## Profiling And Logging

- Build profiling notes: [docs/profiling.md](profiling.md)
- Logging conventions: [docs/logging_guidlines.md](logging_guidlines.md)

The logging document keeps the historical `logging_guidlines.md` filename.
Preserve existing links unless the file is renamed in a coordinated cleanup.

## Release Process

The release pipeline and packaging details are documented in
[docs/linux_binary_distribution.md](linux_binary_distribution.md).

Tagging example:

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

Commit messages should be short, imperative, and sentence case, for example
`Add substeps` or `Split jacobi implementations`.
