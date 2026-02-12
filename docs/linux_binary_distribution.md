# Linux Binary Distribution

This repository now provides a Linux-only release pipeline for distributing prebuilt binaries to other developers.

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

Runs on `ubuntu-24.04` and performs:
- Installs required system tools for vcpkg `python3` (`autoconf`, `automake`, `autoconf-archive`, `ninja-build`)
- Installs and uses `gcc-13`/`g++-13` for C++23 `<format>` support
- Configure (`cmake --preset=vcpkg -DCMAKE_BUILD_TYPE=Release -DSSP4SIM_BUILD_TEST=ON -DSSP4SIM_BUILD_PYTHON_API=ON -DVCPKG_MANIFEST_FEATURES=python-api`)
- Build (`cmake --build build --config Release`)
- Test (`./build/tests/ssp4sim_tests`)

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

1. Download the alias tarball, wheel alias, and `SHA256SUMS` from the latest release.
2. Verify checksums:
   ```bash
   sha256sum -c SHA256SUMS
   ```
3. Extract:
   ```bash
   tar -xzf ssp4sim-linux-x86_64-latest.tar.gz
   ```
4. Run:
   ```bash
   ./ssp4sim/bin/sim_app ./ssp4sim/resources/embrace/embrace.json
   ```
5. Optionally add `ssp4sim/bin` to `PATH`.
6. Install the Python API wheel:
   ```bash
   pip install pyssp4sim-0.0.0+lat-cp39-abi3-linux_x86_64.whl
   ```
