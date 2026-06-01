# Build From Source
<!-- Layer: 03-implementation -->


This guide covers configuring and building `ssp4sim` from a source checkout.
For release artifacts, see [Installation](installation.md). For run commands
after a successful build, see [Usage](usage.md).
This page is the canonical source for source-build and test command blocks;
other docs link here instead of repeating the same sequences.

## Recommended Path

The most repeatable local build is the container workflow. It matches the Linux
release workflow and avoids host compiler or vcpkg drift.

Host commands:

```bash
./resources/containers/build_ubuntu22_gcc13_container.sh
./resources/containers/shell_ubuntu22_gcc13_container.sh
```

Inside the container shell:

```bash
cmake --preset=vcpkg
cmake --build build
./build/tests/lib/ssp4sim_tests
```

Success criteria:

- `cmake --build build` completes without errors.
- `./build/tests/lib/ssp4sim_tests` exits successfully.
- `./build/public/ssp4sim_app/sim_app ./resources/embrace/embrace.json`
  produces a result artifact at the configured output path.

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

The repository includes a build container based on Ubuntu 22.04 with GCC 13
that mirrors the verified environment in
[`.github/workflows/linux-release.yml`](../.github/workflows/linux-release.yml).
Use `resources/containers/run_ubuntu22_gcc13_container.sh` for non-interactive
commands and CI-style runs.

Container file overview:

- `resources/containers/ubuntu22-gcc13/Containerfile` defines the reusable Ubuntu 22.04 + GCC 13 image and installs `vcpkg`.
- The image also includes the Python packaging modules used by the release wheel step, so packaging can run offline once the image is built.
- `resources/containers/build_ubuntu22_gcc13_container.sh` builds that image with Podman or Docker.
- `resources/containers/shell_ubuntu22_gcc13_container.sh` opens an interactive shell with the repository bind-mounted at `/work`.
- `resources/containers/run_ubuntu22_gcc13_container.sh` runs a non-interactive command in the same container image and is used by CI.

The helper scripts prefer Podman when it can initialize cleanly, but fall back
to Docker if Podman is present yet unusable in the local environment.

The shell helper bind-mounts the host repository root into the container at
`/work`, so edits in either place affect the same files on the host. When the
helper uses Podman, it starts the container with `--userns keep-id` so the
bind-mounted repository remains writable as your host user. If you still see
`Permission denied` under `/work`, verify that you launched the shell via the
helper script instead of a manual `podman run`.

The Linux release workflow uses the same container image in GitHub Actions
instead of duplicating native toolchain setup in workflow YAML. Native Linux
build-environment changes should therefore be made in the `Containerfile`, not
redefined separately in CI.

The container keeps the toolchain aligned with the release workflow, but still uses the repository's current `vcpkg.json` and CMake options.

## Configure And Build

After entering the container shell, run the same preset and build commands
shown in the recommended path. Keep using the repository preset when adding
cache options so the build continues to use the vcpkg toolchain.

## Custom Build Directory

If you want the build tree somewhere other than `./build`, configure CMake with
an explicit source and build directory instead of the preset. For example, CI
uses `build_cont`:

```bash
cmake -S . -B build_cont \
  -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$VCPKG_ROOT/scripts/buildsystems/vcpkg.cmake"
cmake --build build_cont
./build_cont/tests/lib/ssp4sim_tests
```

Keep using the same build directory for later commands such as
`cmake --build`, `ctest`, and `cmake --install`.

## Build Types And Logging Path

```bash
cmake --preset=vcpkg -DCMAKE_BUILD_TYPE=Debug
cmake --preset=vcpkg -DCMAKE_BUILD_TYPE=Release

cmake --preset=vcpkg -DSSP4SIM_LOG_HOT_PATH=ON
cmake --preset=vcpkg -DSSP4SIM_LOG_HOT_PATH=OFF
```

## Enable C++ Tests

See tests/README.md for test-running caveats.

Enable the C++ test target by adding `-DSSP4SIM_BUILD_TEST=ON` to the preset
command, then rebuild and run `./build/tests/lib/ssp4sim_tests`.

Some test fixtures are stored as expanded FMU/SSP directories instead of `.fmu`/`.ssp` archives to make resource diffs and version handling easier. Tests should accept either layout when resolving fixture paths.

More test-suite detail is in [tests/README.md](../tests/README.md).

## Build Python API (Editable Install)

Use the same Python version for CMake and `pip`. The Python API requires a
Release build, the editable version is derived from Git tags
(`setuptools-scm`), and the native module targets the CPython Limited API
(`Py_LIMITED_API=0x03090000`) to produce an `abi3` extension.

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
