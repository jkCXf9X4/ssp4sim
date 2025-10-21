# Repository Guidelines

## Project Structure & Module Organization
- `lib/include` provides public headers; keep implementations in the mirrored folders under `lib/src`.
- CLI sources sit in `public/ssp4sim_app`, and bindings in `public/python_api`; touch both when changing shared interfaces.
- Simulation inputs live in `resources/`; treat `results/` as scratch space and leave large outputs untracked.
- Tests mirror the library layout in `tests/{utils,sim,...}`; add fixtures next to the code under test.

## Build, Test, and Development Commands
```bash
cmake --preset=vcpkg                              # configure toolchain + deps
cmake -S . -B build -DSSP4CPP_BUILD_TEST=ON       # configure with tests
cmake --build build                               # build libs and apps
./build/public/ssp4sim_app/ssp4sim_app resources/embrace/embrace.json
cmake --build build --target test_1 && ./build/tests/test_1  # Catch2 suite
```
- Use the preset for reproducible dependency resolution and rerun configuration after toggling options like `SSP4CPP_BUILD_PYTHON`.

## Coding Style & Naming Conventions
- Use modern C++23 patterns with 4-space indentation, brace-on-new-line for namespaces/classes, and concise `camelCase` helper names.
- Group includes as `<system>` then `"project"` headers, and promote shared logic into `utils`.
- Keep declarations in `lib/include` with paired implementations in `lib/src`; add short Doxygen blocks for public APIs.
- Python glue should follow PEP 8 and reuse `python3 -m venv venv`.

## Testing Guidelines
- Catch2 drives the suite; name sources `test_<feature>.cpp` and tag cases (e.g., `[Node]`) for focused runs.
- Reconfigure with `SSP4CPP_BUILD_TEST=ON`, rebuild, and execute `./build/tests/test_1` (or `ctest --test-dir build` when stable) before submitting.
- Store fixtures in `tests/references` and reuse assets from `resources/` for deterministic inputs.
- Cover success and failure paths; call out known gaps in the PR description if they remain.

## Commit & Pull Request Guidelines
- Favor short, imperative summaries (e.g., `Add Tarjan Graph Utilities`) similar to the existing history; wrap body text near 72 chars.
- Optional module prefixes like `[utils]` keep the log scannable; reference issues in the footer (`Refs #123`).
- PRs should explain intent, list manual or automated test results, and note any new resources; share screenshots or logs for CLI changes.
- Request review once tests pass and park unfinished ideas in `todo.md` instead of leaving them undocumented.

## Tooling & Data Tips
- Helper scripts in `scripts/` (`compare_with_ref.py`, `plot.py`) validate outputs against `tests/references`.
- Keep generated experiment data in `results/` and track reproducibility notes in `profiling.md` when the workflow changes.
