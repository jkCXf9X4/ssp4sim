# The most important guidelines

1. Don’t assume. Don’t hide confusion. Surface tradeoffs.

2. Minimum code that solves the problem. Nothing speculative

3. Touch only what you must. Clean up only your own mess.

4. Define success criteria. Loop until verified.

# Repository Guidelines

This file is for agent-specific instructions. Project information that is useful
to developers and users should live in the public documentation and be linked
from here.

## Canonical Documentation

- Project overview and user entry points: [`readme.md`](readme.md)
- Running the CLI and Python API: [`docs/usage.md`](docs/usage.md)
- Local build commands: [`docs/build_from_source.md`](docs/build_from_source.md)
- Development workflows, repository layout, coding style, dependencies, generated
  data, and contribution conventions: [`docs/development.md`](docs/development.md)
- Test architecture, commands, reference fixtures, and expected failures:
  [`tests/README.md`](tests/README.md)
- Runtime configuration: [`docs/configuration.md`](docs/configuration.md)
- Profiling and logging: [`docs/profiling.md`](docs/profiling.md),
  [`docs/logging_guidlines.md`](docs/logging_guidlines.md)

## Agent Workflow

- After each task, update relevant documentation when behavior, commands,
  architecture, expected failures, or workflow assumptions changed. Prefer
  `readme.md`, `tests/README.md`, or `docs/` depending on scope.
- Avoid monkey patching or similar workarounds. Find and fix the root cause for
  errors whenever the repository context makes that practical.
- Keep repository context narrow. Start with the files directly involved in the
  task and expand only when the local design or failure mode requires it.
- Avoid bulk-reading generated artifacts, vendored code, or large fixture
  directories unless the task explicitly depends on them.
- Before changing files under `tests/reference_ssp/`, read
  `tests/reference_ssp/AGENTS.md`.

## Quick Commands

Use the canonical docs above for details. Common commands:

```bash
cmake --preset=vcpkg
cmake --build build
./build/tests/lib/ssp4sim_tests
pytest -q tests/python
```
