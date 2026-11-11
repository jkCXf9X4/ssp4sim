---
id: INFO-028
type: info
title: CI Test Stages
summary: Three GitHub Actions stages — PR check, merge gate, release validation — each with its own compile, test, and gate
date: 2026-11-11
status: current
---

# CI Test Stages

Three CI stages run on GitHub Actions; each compiles its targets, runs its tests, and enforces a gate.

- **PR check** (pull request to main):
  - Compiles the C++ library and test binary.
  - Runs C++ unit and integration tests (core, utils, high-level smoke).
  - Runs the Python CLI smoke test only.
  - Budget under 5 minutes; gate: all C++ tests and the CLI smoke test pass.
- **Merge gate** (merge to main):
  - Compiles the C++ library, Python extension, and test binary.
  - Runs the full C++ suite and the full Python suite (reference-SSP sweep, CLI smoke test).
  - Budget under 15 minutes; gate: all tests pass, documented xfails excluded.
- **Release validation** (tag push):
  - Runs the full merge-gate suite.
  - Validates packaging (tarball and wheel) and smokes the packaged artifacts.
  - Gate: all tests pass plus packaging verification.
- **Canonical runner:** CTest drives the C++ tests; the Python suite runs under pytest.
- **Container:** the pipeline image is built from `containers/ubuntu22-gcc13/Containerfile`.
- **Detail caveat:** workflow files in `.github/workflows/` may carry more than this leaf records; this reflects known behavior.
