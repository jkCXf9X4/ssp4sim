# Linux Binary Distribution

This repository provides a Linux-only release pipeline for distributing
prebuilt binaries and a Python wheel to other developers.

## Stage 1: Distribution target

Release artifact format (same tag/SHA):
- `ssp4sim-linux-x86_64-<version>.tar.gz`
- `pyssp4sim-<version>-*.whl`
- `ssp4sim-linux-x86_64-latest.tar.gz` (alias)
- `pyssp4sim-0.0.0+lat-cp39-abi3-linux_x86_64.whl` (alias)

Tarball contents are installed from CMake into:
- `bin/`
- `lib/`
- `include/`
- `python/`
- `resources/`
- metadata files ([`readme.md`](../readme.md), [`LICENSE`](../LICENSE), `RELEASE.txt`)

## Stage 2: CI build and test

Workflow: [`.github/workflows/linux-release.yml`](../.github/workflows/linux-release.yml)
Template inputs for generated release files:
- [`scripts/release/setup_release.py.template`](../scripts/release/setup_release.py.template)
- [`scripts/release/RELEASE.txt.template`](../scripts/release/RELEASE.txt.template)

Runs on `ubuntu-22.04` and performs:

- Builds the reusable Ubuntu 22.04 + GCC 13 container image from
  [`containers/ubuntu22-gcc13/Containerfile`](../containers/ubuntu22-gcc13/Containerfile)
- Runs configure, build, test, install, and packaging commands inside that
  container
- Uses the same configure/build/test commands documented in
  [Build From Source](build_from_source.md) and [Tests](../tests/README.md)

## Stage 3: Install to staging directory

The workflow stages install output with:
```bash
cmake --install build --prefix dist/ssp4sim
```

Install rules are defined in:
- [`CMakeLists.txt`](../CMakeLists.txt)
- [`lib/CMakeLists.txt`](../lib/CMakeLists.txt)
- [`public/ssp4sim_app/CMakeLists.txt`](../public/ssp4sim_app/CMakeLists.txt)
- [`public/python_api/CMakeLists.txt`](../public/python_api/CMakeLists.txt)

## Stage 4: Package artifacts

The workflow creates:
- `dist/ssp4sim-linux-x86_64-<version>.tar.gz`
- `dist/pyssp4sim-<version>-*.whl`
- `dist/ssp4sim-linux-x86_64-latest.tar.gz`
- `dist/pyssp4sim-0.0.0+lat-cp39-abi3-linux_x86_64.whl`
- `dist/SHA256SUMS`

`RELEASE.txt` is generated before packaging and includes:
- version
- commit SHA
- UTC build timestamp

## Stage 5: Publish artifacts

On push tags matching `v*`, the workflow publishes versioned assets, tarball alias, wheel alias, and checksum file to GitHub Releases.

For manual runs (`workflow_dispatch`), artifacts are uploaded to the workflow run as build artifacts.

## Stage 6: Developer consumption

Use the release tarball and wheel the same way described in [Installation](installation.md)
and [Usage](usage.md). The release assets expose `bin/sim_app` and
`pyssp4sim`; the install and run commands are documented there.

## Local Release Parity

Use the same container scripts locally when investigating release failures.
The command sequence is the same as the build and test workflow described in
[Build From Source](build_from_source.md) and [Tests](../tests/README.md).
