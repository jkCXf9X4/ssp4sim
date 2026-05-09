# Build From Source

This guide covers configuring and building `ssp4sim` from a source checkout.
For release artifacts, see [Installation](installation.md). For run commands
after a successful build, see [Usage](usage.md).
This page is the canonical source for source-build and test command blocks;
other docs link here instead of repeating the same sequences.

## Recommended Path

The most repeatable local build is the container workflow. It matches the Linux
release workflow and avoids host compiler or vcpkg drift:

```bash
./containers/build_ubuntu22_gcc13_container.sh
./containers/shell_ubuntu22_gcc13_container.sh
cmake --preset=vcpkg -DSSP4SIM_BUILD_TEST=ON
cmake --build build
./build/tests/lib/ssp4sim_tests
```

Success criteria:

- `cmake --build build` completes without errors.
- `./build/tests/lib/ssp4sim_tests` exits successfully.
- `./build/public/ssp4sim_app/sim_app ./resources/embrace/embrace.json`
  produces a result CSV at the configured output path.

## Prepare A Source Build Environment

Prerequisites:

- CMake (with presets support)
- A C++23-capable compiler
- [vcpkg](https://github.com/microsoft/vcpkg) configured for this repository.
  `CMakePresets.json` expects `VCPKG_ROOT` to point at the vcpkg checkout.
- Ninja, because the repository preset uses the Ninja generator.

Known Debian/Ubuntu packages needed by vcpkg builds:

```bash
sudo apt install -y ninja-build autoconf automake autoconf-archive cmake build-essential libtool pkg-config
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

The helper scripts prefer Podman when it can initialize cleanly, but fall back
to Docker if Podman is present yet unusable in the local environment.

The shell script bind-mounts the host repository root into the container at `/work`, so edits in either place affect the same files on the host. It does not create a separate host mount point:

When the shell helper uses Podman, it now starts the container with `--userns keep-id` so the bind-mounted repository remains writable as your host user. If you still see `Permission denied` under `/work`, verify you launched the shell via the helper script instead of a manual `podman run`.

The Linux release workflow now builds and uses this same container image in GitHub Actions instead of duplicating the native toolchain setup in workflow YAML. Native Linux build-environment changes should therefore be made in the `Containerfile`, not redefined separately in CI.

The container keeps the toolchain aligned with the release workflow, but still uses the repository's current `vcpkg.json` and CMake options.

## Configure And Build

Use the repository preset. Keep using the preset when adding cache options so
the build continues to use the vcpkg toolchain:

```bash
cmake --preset=vcpkg
cmake --build build
```

## Build Types And Logging Path

```bash
cmake --preset=vcpkg -DCMAKE_BUILD_TYPE=Debug
cmake --preset=vcpkg -DCMAKE_BUILD_TYPE=Release

cmake --preset=vcpkg -DSSP4SIM_LOG_HOT_PATH=ON
cmake --preset=vcpkg -DSSP4SIM_LOG_HOT_PATH=OFF
```

## Enable C++ Tests

`ctest --test-dir build/tests` is currently unreliable. Run the `./build/tests/lib/ssp4sim_tests` binary directly:

```bash
cmake --preset=vcpkg -DSSP4SIM_BUILD_TEST=ON
cmake --build build
./build/tests/lib/ssp4sim_tests
```

Some test fixtures are stored as expanded FMU/SSP directories instead of `.fmu`/`.ssp` archives to make resource diffs and version handling easier. Tests should accept either layout when resolving fixture paths.

More test-suite detail is in [tests/README.md](../tests/README.md).

## Build Python API (Editable Install)

Use the same Python version for CMake and `pip`. Python API requires a Release build.
`pyssp4sim` version is derived from Git tags (`setuptools-scm`) for source/editable installs.
The native module is built against CPython Limited API (`Py_LIMITED_API=0x03090000`) and produces an `abi3` extension.

```bash
python3.11 -m venv venv
. ./venv/bin/activate
pip install -r ./requirements.txt

cmake --preset=vcpkg -DCMAKE_BUILD_TYPE=Release -DSSP4SIM_BUILD_PYTHON_API=ON -DSSP4SIM_LOG_HOT_PATH=OFF

cmake --build build
pip install -e ./build/public/python_api
```

Run Python tests after the Python API has been built:

```bash
pytest -q tests/python
```

CLI and Python usage examples are in [Usage](usage.md).
