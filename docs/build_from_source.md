# Build From Source

This guide covers local build, test, and run commands for `ssp4sim`.

## Prepare A Source Build Environment

Prerequisites:

- CMake (with presets support)
- A C++23-capable compiler
- [vcpkg](https://github.com/microsoft/vcpkg) configured for this repository

Known packages needed by vcpkg builds:

```bash
sudo apt install -y ninja-build autoconf automake autoconf-archive cmake build-essential pkg-config
```

Clone the repository:

```bash
git clone --recursive git@github.com:jkCXf9X4/ssp4sim.git
cd ssp4sim
```

If you already cloned without submodules:

```bash
git submodule update --init --recursive
```

## Build Container Workflow

The repository includes a build container based on Ubuntu 22.04 with GCC 13 that mirrors the verified environment in [`.github/workflows/linux-release.yml`](../.github/workflows/linux-release.yml):

```bash
./containers/build_ubuntu22_gcc13_container.sh
./containers/shell_ubuntu22_gcc13_container.sh
```

Container file overview:

- `containers/ubuntu22-gcc13/Containerfile` defines the reusable Ubuntu 22.04 + GCC 13 image and installs `vcpkg`.
- `containers/build_ubuntu22_gcc13_container.sh` builds that image with Podman or Docker.
- `containers/shell_ubuntu22_gcc13_container.sh` opens an interactive shell with the repository bind-mounted at `/work`.
- `containers/run_ubuntu22_gcc13_container.sh` runs a non-interactive command in the same container image and is used by CI.

The shell script bind-mounts the host repository root into the container at `/work`, so edits in either place affect the same files on the host. It does not create a separate host mount point:

When the shell helper uses Podman, it now starts the container with `--userns keep-id` so the bind-mounted repository remains writable as your host user. If you still see `Permission denied` under `/work`, verify you launched the shell via the helper script instead of a manual `podman run`.

The Linux release workflow now builds and uses this same container image in GitHub Actions instead of duplicating the native toolchain setup in workflow YAML. Native Linux build-environment changes should therefore be made in the `Containerfile`, not redefined separately in CI.

The container keeps the toolchain aligned with the release workflow, but still uses the repository's current `vcpkg.json` and CMake options.

## Configure And Build

Use the repository preset:

```bash
cmake --preset=vcpkg
cmake --build build
```

## Build Types And Logging Path

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

cmake -S . -B build -DSSP4SIM_LOG_HOT_PATH=ON
cmake -S . -B build -DSSP4SIM_LOG_HOT_PATH=OFF
```

## Build And Run Tests

`ctest --test-dir build/tests` is currently unreliable. Run the `./build/tests/lib/ssp4sim_tests` binary directly:

```bash
cmake -B build -S . -DSSP4SIM_BUILD_TEST=ON
cmake --build build
./build/tests/lib/ssp4sim_tests
```

Some test fixtures are stored as expanded FMU/SSP directories instead of `.fmu`/`.ssp` archives to make resource diffs and version handling easier. Tests should accept either layout when resolving fixture paths.

## Build Python API (Editable Install)

Use the same Python version for CMake and `pip`. Python API requires a Release build.
`pyssp4sim` version is derived from Git tags (`setuptools-scm`) for source/editable installs.
The native module is built against CPython Limited API (`Py_LIMITED_API=0x03090000`) and produces an `abi3` extension.

```bash
python3.11 -m venv venv
. ./venv/bin/activate
pip install -r ./requirements.txt

cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DSSP4SIM_BUILD_PYTHON_API=ON -DVCPKG_MANIFEST_FEATURES=python-api -DSSP4SIM_LOG_HOT_PATH=OFF

cmake --build build
pip install -e ./build/public/python_api
```

## Run The CLI App

```bash
./build/public/ssp4sim_app/sim_app ./resources/embrace/embrace.json
```
