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
    git clone git@github.com:jkCXf9X4/ssp4cpp.git
    git submodule update --init --recursive 
    or
    git clone --recursive git@github.com:jkCXf9X4/ssp4cpp.git
    ```

2.  Configure the build using the provided CMake preset (requires [vcpkg](https://github.com/microsoft/vcpkg)):
    ```bash
    cmake --preset=vcpkg
    ```

3.  Build the project:
    ```bash
    cmake --build build
    ```

4. Release/debug
```
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

Possible dependencies
sudo apt install ninja-build

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
cmake --build build && ./build/tests/test_1
```
ctest --test-dir build/tests currently malfunctions...

## Building python api
Make sure to use the same version of python as you build for. First build the python bindings
```bash
cmake -B build -S . -DSSP4SIM_BUILD_PYTHON_API=ON
cmake --build build


python3 -m venv venv
. ./venv/bin/activate
pip install -r requirements.txt
pip install -e ./build/public/python_api
```


## Contributing
Contributions are welcome! Please open an issue or submit a pull request if you have any improvements or suggestions.

## License
This project is released under the MIT license. See [LICENCE](LICENCE) for details.
