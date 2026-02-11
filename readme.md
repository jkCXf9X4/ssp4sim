# SSP4SIM

SSP4SIM is a C++23 library and application for simulating Structure and Parameterization (SSP) archives. The goal is to create a small experimental simulation engine to develop and test novel (and old) simulation strategies.

It currently supports the following execution strategies:
- Gauss-Jacobi
- Gauss-Seidel 

It currently supports SSP1.0 and FMI2.0 models.

See the [SSP standard](https://ssp-standard.org) for more information about the file format.

Builds upon the [SSP4CPP](https://github.com/jkCXf9X4/ssp4cpp) XML deserializer


## Project Structure

The project is organized into the following directories:

- `3rdParty`: Contains third-party libraries and dependencies.
- `lib`: Contains the simulation engine, which is responsible for loading and executing SSP files.
- `public`: Contains main application and python api.
- `resources`: Contains SSP files and other resources used by the examples and tests.
- `tests`: Contains unit tests for the SSP4SIM library.


## Getting started
1.  Clone the repository and initialize submodules:
    ```bash
    git clone git@github.com:jkCXf9X4/ssp4sim.git
    git submodule update --init --recursive 
    or
    git clone --recursive git@github.com:jkCXf9X4/ssp4sim.git
    ```

2.  Configure the build using the provided CMake preset (requires [vcpkg](https://github.com/microsoft/vcpkg)):
    ```bash
    cmake --preset=vcpkg
    ```

3.  Build the project:
    ```bash
    cmake --build build
    ```

## Linux release binaries
Tagged releases (`v*`) publish dual artifacts from the same commit/tag:
- Linux bundle: `ssp4sim-linux-x86_64-vX.Y.Z.tar.gz`
- Python wheel: `pyssp4sim-X.Y.Z-*.whl`

The Linux tarball contains:
- `bin/sim_app`
- `python/pyssp4sim` (Python API package with native extension)
- `readme.md`, `LICENSE`, `version.txt`, `RELEASE.txt`

Download and run:
```bash
curl -L -o ssp4sim-linux.tar.gz \
  https://github.com/jkCXf9X4/ssp4sim/releases/latest/download/ssp4sim-linux-x86_64-vX.Y.Z.tar.gz

curl -L -o SHA256SUMS \
  https://github.com/jkCXf9X4/ssp4sim/releases/latest/download/SHA256SUMS

sha256sum -c SHA256SUMS

mkdir -p $HOME/.local/opt/ssp4sim
tar -xzf ssp4sim-linux.tar.gz -C $HOME/.local/opt

$HOME/.local/opt/ssp4sim/bin/sim_app ./resources/embrace/embrace.json
```

Add to PATH:
```bash
export PATH="$HOME/.local/opt/ssp4sim/bin:$PATH"
```

Runtime notes:
- Releases are currently built on Ubuntu 24.04.
- The binary links `libstdc++` statically, but system compatibility can still depend on glibc and other runtime components.

Release pipeline details: `docs/linux_binary_distribution.md`

Install Python API from release wheel:
```bash
pip install ./pyssp4sim-X.Y.Z-*.whl
```

## Release/debug
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release

cmake -S . -B build -DSSP4SIM_LOG_HOT_PATH=ON
cmake -S . -B build -DSSP4SIM_LOG_HOT_PATH=OFF
```

Possible dependencies
sudo apt install -y ninja-build autoconf automake autoconf-archive

Release!
```
  git tag v0.1.1
  git push origin v0.1.1

```

## Running examples
After building, you can run the SSP simulation engine:
```bash
./build/public/ssp4sim_app/sim_app ./resources/embrace/embrace.json
```
This will run a simple simulation using one of the example ssps.

## Running tests
To run the tests, you first need to enable the `SSP4SIM_BUILD_TEST` option in CMake:
```bash
cmake -B build -S . -DSSP4SIM_BUILD_TEST=ON
cmake -B build -S . -DSSP4SIM_BUILD_TEST=OFF
cmake --build build && ./build/tests/ssp4sim_tests
```
ctest --test-dir build/tests currently malfunctions...

## Building python api (quickest path)
Make sure to use the same Python version for CMake and `pip`. The Python API requires a Release build.
```bash
python3.11 -m venv venv
. ./venv/bin/activate
pip install -r requirements.txt

cmake -B build -S . -DSSP4SIM_BUILD_PYTHON_API=ON -DVCPKG_MANIFEST_FEATURES=python-api -DCMAKE_BUILD_TYPE=Release -DSSP4SIM_LOG_HOT_PATH=OFF
cmake --build build

# Install as a local (editable) module
pip install -e ./build/public/python_api
```


## Contributing
Contributions are welcome! Please open an issue or submit a pull request if you have any improvements or suggestions.

## License
This project is released under the MIT license. See [LICENCE](LICENCE) for details.
