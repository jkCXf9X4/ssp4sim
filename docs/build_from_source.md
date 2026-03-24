# Build From Source

This guide covers local builds of `ssp4sim` for CLI, tests, and Python API.

## Prerequisites

- CMake (with presets support)
- A C++23-capable compiler
- [vcpkg](https://github.com/microsoft/vcpkg) configured for this repository

Known packages needed by vcpkg builds:

```bash
sudo apt install -y ninja-build autoconf automake autoconf-archive cmake build-essential pkg-config
```

## Clone

```bash
git clone --recursive git@github.com:jkCXf9X4/ssp4sim.git
cd ssp4sim
```

If you already cloned without submodules:

```bash
git submodule update --init --recursive
```

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

`ctest --test-dir build/tests` is currently unreliable. Run the `build/tests/ssp4sim_tests` binary directly:

```bash
cmake -B build -S . -DSSP4SIM_BUILD_TEST=ON
cmake --build build
./build/tests/ssp4sim_tests
```

## Build Python API (Editable Install)

Use the same Python version for CMake and `pip`. Python API requires a Release build.
`pyssp4sim` version is derived from Git tags (`setuptools-scm`) for source/editable installs.
The native module is built against CPython Limited API (`Py_LIMITED_API=0x03090000`) and produces an `abi3` extension.

```bash
python3.11 -m venv venv
. ./venv/bin/activate
pip install -r ./requirements.txt

cmake -B build -S .  -DCMAKE_BUILD_TYPE=Release -DSSP4SIM_LOG_HOT_PATH=OFF

cmake --build build
pip install -e ./build/public/python_api
```

## Run The CLI App

```bash
./build/public/ssp4sim_app/sim_app ./resources/embrace/embrace.json
```
