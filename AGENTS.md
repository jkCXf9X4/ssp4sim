# Repository Guidelines

- After each task, update relevant documentation when behavior, commands,
  architecture, expected failures, or workflow assumptions changed. Prefer
  `README.md`, `tests/README.md`, or `docs/` depending on scope.
- Avoid monkey patching or similar workarounds. Find and fix the root cause for
  errors whenever the repository context makes that practical.

Keep repository context narrow. Start with the files directly involved in the
task and expand only when the local design or failure mode requires it. Avoid
bulk-reading generated artifacts, vendored code, or large fixture directories
unless the task explicitly depends on them.

## Project Structure & Module Organization
- `lib/`: core C++23 simulation engine (execution strategies, graph, utils, handlers).
- `public/`: shipped entry points, including `public/ssp4sim_app/` and `public/python_api/`.
- `tests/lib/`: Catch2-based C++ unit/integration tests split by layer
  (`core/`, `utils/`, and one `high_level/` smoke test).
- `tests/python/`: pytest-based high-level workflow tests.
- `tests/reference_ssp/`: nested reference fixture repository used by high-level
  tests. Read its own `AGENTS.md` before changing files inside it.
- `resources/`: SSP/SSD/SSM/SSV examples, reference inputs, and sample scenarios.
- `scripts/`: helper scripts for filtering and comparing results.
- `3rdParty/`: vendored dependencies (ssp4cpp, fmi4c, etc.).

## Build, Test, and Development Commands
- Configure with vcpkg preset:
  ```bash
  cmake --preset=vcpkg
  ```
- Build all targets:
  ```bash
  cmake --build build
  ```
- Run the CLI app (example):
  ```bash
  ./build/public/ssp4sim_app/sim_app ./resources/embrace/embrace.json
  ```
- Enable and run tests:
  ```bash
  cmake -B build -S . -DSSP4SIM_BUILD_TEST=ON
  cmake --build build && ./build/tests/lib/ssp4sim_tests
  ```
- Build Python API (release only) and install editable:
  ```bash
  cmake -B build -S . -DSSP4SIM_BUILD_PYTHON_API=ON -DVCPKG_MANIFEST_FEATURES=python-api -DCMAKE_BUILD_TYPE=Release -DSSP4SIM_LOG_HOT_PATH=OFF
  cmake --build build
  python3.11 -m venv venv && . ./venv/bin/activate
  pip install -r requirements.txt
  pip install -e ./build/public/python_api
  ```

## Coding Style & Naming Conventions
- C++ formatting follows a 4-space indent with braces on the next line.
- Class names use `PascalCase`; functions and files generally use `snake_case`.
- Keep includes ordered and local headers grouped (see `lib/include/**`).
- No enforced formatter is checked in; match surrounding style closely.
- Keep changes direct and explicit. Minimize duplication, but do not introduce
  abstractions that hide details unnecessarily.
- Prefer repository-owned code in `lib/`, `public/`, `tests/`, `resources/`, and
  `scripts/` over patching vendored dependencies in `3rdParty/`, unless the task
  is explicitly about vendored behavior.
- Treat this as active experimental software: clarity and root-cause fixes are
  preferred over compatibility shims or broad defensive workarounds.

## Testing Guidelines
- Test framework: Catch2 (`tests/CMakeLists.txt`).
- Prefer `test_*.cpp` naming and small, focused C++ cases under
  `tests/lib/core/` or `tests/lib/utils/`.
- Keep `tests/lib/high_level/` to one top-level C++ smoke path. Put reference
  sweeps and detailed result comparisons in pytest under `tests/python/`.
- `ctest --test-dir build/tests` is noted as unreliable; run the test binary directly.
- Choose the smallest validation that proves the change. Use focused Catch2
  tests, focused pytest tests, or one reference SSP simulation before running
  broader suites.
- For Python tests, collect only `tests/python` unless the task explicitly asks
  for another tree:
  ```bash
  pytest tests/python
  ```
- Reference SSP fixtures under `tests/reference_ssp/` may contain a nested git
  repository and generated `build/` content. Do not edit generated outputs unless
  the task is explicitly about fixture generation or expected reference data.
  When editing this tree, follow `tests/reference_ssp/AGENTS.md`.

## Commit & Pull Request Guidelines
- Commit messages are short, imperative, sentence case (e.g., "Add substeps", "Split jacobi implementations").
- PRs should include a clear description, the tests you ran, and reference related issues.
- Attach before/after output or plots when changing simulation results or reference data.

## Configuration & Dependencies
- CMake presets (`CMakePresets.json`) expect vcpkg; see `vcpkg.md` for setup.
- Optional flags: `SSP4SIM_BUILD_TEST`, `SSP4SIM_BUILD_PYTHON_API`, `SSP4SIM_LOG_HOT_PATH`.
- vcpkg manifest feature `python-api` is required when `SSP4SIM_BUILD_PYTHON_API=ON`.
- Use the repo-local `venv` for Python commands when it exists. Prefer
  `. venv/bin/activate && <command>` or `venv/bin/python <command>` over the
  system Python for workflow scripts and pytest.
- Do not casually rewrite dependency setup. Python dependencies, vcpkg features,
  and platform-specific FMU tooling are part of the reproducible environment.
- Many FMU and SSP workflows assume Linux `x86_64` binaries. Preserve executable
  permissions for libraries under `binaries/`, and prefer copying fixtures to a
  temporary location when a loader or unpacking step may mutate files.
- Treat `build/`, unpacked archives, result CSVs, logs, and FMU/SSP binaries as
  generated or fixture data. Do not normalize, repackage, or rewrite them unless
  the requested change explicitly requires it.

## Documentation Guidelines
- Keep documentation short, focused, and tied to the actual build, simulation,
  testing, or fixture workflow.
- Treat `README.md` as the landing page for repository structure and common
  usage. Put test architecture and suite behavior in `tests/README.md`.
- Put cross-cutting design, workflow, or troubleshooting notes in `docs/` when
  they are too detailed for the README.
- When commands, paths, expected failures, or generated fixture behavior change,
  update references so the documented flow remains accurate.
