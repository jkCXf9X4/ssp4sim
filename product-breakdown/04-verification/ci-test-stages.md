# CI Test Stages
<!-- Layer: 04-verification -->
<!-- Stable ID: VER-CI-001 -->

## Description

This artifact documents which tests run at each CI stage and what gates each stage enforces.

## Stage: PR Check

Triggered on pull request to main branch.

- Compile C++ library and test binary
- Run C++ unit and integration tests (core, utils, high_level smoke)
- Python tests: limited set (CLI smoke test only)
- Expected time: < 5 minutes
- Gate: All C++ tests must pass; Python CLI smoke test must pass

## Stage: Merge Gate

Triggered on merge to main branch.

- Compile C++ library, Python extension, test binary
- Full C++ test suite
- Full Python test suite (reference SSP sweep, CLI smoke test)
- Expected time: < 15 minutes
- Gate: All tests must pass (documented xfails excluded)

## Stage: Release Validation

Triggered on tag push.

- Full merge gate suite
- Packaging validation (tarball + wheel)
- Smoke test from packaged artifacts
- Gate: All tests pass + packaging verification

## Current Limitations

- CI workflow files in `.github/workflows/` may contain additional detail — this doc reflects known behavior
- Docker container build is part of the release pipeline (from container/ubuntu22-gcc13/Containerfile)
- ctest integration is unreliable — test binary execution is the canonical method

## Traceability

- Backward: Traces to acceptance criteria in `product-breakdown/04-verification/acceptance-criteria.md`.
- Sources: `.github/workflows/linux-release.yml`, `docs/linux_binary_distribution.md`, `docs/build_from_source.md`.