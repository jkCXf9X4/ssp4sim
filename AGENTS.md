# Repository Guidelines

After each task update and improve the documentation such as, but not limited to, readme.md or docs/

Avoid monkey patching or similar, always try to find root cause for errors

## Project Structure & Module Organization
- `lib/`: core C++23 simulation engine (execution strategies, graph, utils, handlers).
- `public/`: shipped entry points, including `public/ssp4sim_app/` and `public/python_api/`.
- `tests/`: Catch2-based unit/integration tests (e.g., `tests/utils/test_*.cpp`).
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
  cmake --build build && ./build/tests/ssp4sim_tests
  ```
- Build Python API (release only) and install editable:
  ```bash
  cmake -B build -S . -DSSP4SIM_BUILD_PYTHON_API=ON -DCMAKE_BUILD_TYPE=Release -DSSP4SIM_LOG_HOT_PATH=OFF
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

## Testing Guidelines
- Test framework: Catch2 (`tests/CMakeLists.txt`).
- Prefer `test_*.cpp` naming and small, focused cases under `tests/utils/` or `tests/sim/`.
- `ctest --test-dir build/tests` is noted as unreliable; run the test binary directly.

## Commit & Pull Request Guidelines
- Commit messages are short, imperative, sentence case (e.g., "Add substeps", "Split jacobi implementations").
- PRs should include a clear description, the tests you ran, and reference related issues.
- Attach before/after output or plots when changing simulation results or reference data.

## Configuration & Dependencies
- CMake presets (`CMakePresets.json`) expect vcpkg; see `vcpkg.md` for setup.
- Optional flags: `SSP4SIM_BUILD_TEST`, `SSP4SIM_BUILD_PYTHON_API`, `SSP4SIM_LOG_HOT_PATH`.
