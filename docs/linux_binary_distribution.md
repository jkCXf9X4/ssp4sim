# Linux Binary Distribution

This repository now provides a Linux-only release pipeline for distributing prebuilt binaries to other developers.

## Stage 1: Distribution target

Release artifact format:
- `ssp4sim-linux-x86_64-<version>.tar.gz`

Tarball contents are installed from CMake into:
- `bin/`
- metadata files (`readme.md`, `LICENSE`, `version.txt`, `RELEASE.txt`)

## Stage 2: CI build and test

Workflow: `.github/workflows/linux-release.yml`

Runs on `ubuntu-24.04` and performs:
- Configure (`cmake --preset=vcpkg -DCMAKE_BUILD_TYPE=Release -DSSP4SIM_BUILD_TEST=ON`)
- Build (`cmake --build build --config Release`)
- Test (`./build/tests/ssp4sim_tests`)

## Stage 3: Install to staging directory

The workflow stages install output with:
```bash
cmake --install build --prefix dist/ssp4sim
```

Install rules are defined in:
- `CMakeLists.txt`
- `lib/CMakeLists.txt`
- `public/ssp4sim_app/CMakeLists.txt`

## Stage 4: Package artifacts

The workflow creates:
- `dist/ssp4sim-linux-x86_64-<version>.tar.gz`
- `dist/SHA256SUMS`

`RELEASE.txt` is generated before packaging and includes:
- version
- commit SHA
- UTC build timestamp

## Stage 5: Publish artifacts

On push tags matching `v*`, the workflow publishes the tarball and checksum file to GitHub Releases.

For manual runs (`workflow_dispatch`), artifacts are uploaded to the workflow run as build artifacts.

## Stage 6: Developer consumption

1. Download the tarball and `SHA256SUMS` from the release.
2. Verify checksums:
   ```bash
   sha256sum -c SHA256SUMS
   ```
3. Extract:
   ```bash
   tar -xzf ssp4sim-linux-x86_64-<version>.tar.gz
   ```
4. Run:
   ```bash
   ./ssp4sim/bin/sim_app ./ssp4sim/resources/embrace/embrace.json
   ```
5. Optionally add `ssp4sim/bin` to `PATH`.
