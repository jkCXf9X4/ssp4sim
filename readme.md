# SSP4SIM

SSP4SIM is a C++23 library and application for simulating Structure and
Parameterization (SSP) archives. The goal is to provide a small experimental
simulation engine for developing and testing simulation strategies.

Current support:

- SSP 1.0 archives and unpacked SSP directories
- FMI 2.0 co-simulation models
- Gauss-Jacobi and Gauss-Seidel execution strategies
- CSV and DuckDB recording as the primary result outputs, with Parquet
  supported for full-fidelity storage snapshots when needed

See the [SSP standard](https://ssp-standard.org) for more information about the file format.

Builds upon the [SSP4CPP](https://github.com/jkCXf9X4/ssp4cpp) XML deserializer
and uses [Quill](https://github.com/odygrd/quill) for logging.

## Quick Start

Run the latest Linux release:

```bash
curl -fLO https://github.com/jkCXf9X4/ssp4sim/releases/latest/download/ssp4sim-linux-x86_64-latest.tar.gz
tar -xzf ssp4sim-linux-x86_64-latest.tar.gz
./ssp4sim/bin/sim_app ./ssp4sim/resources/embrace/embrace.json
```

Build from source and run the C++ tests:

```bash
git clone --recursive git@github.com:jkCXf9X4/ssp4sim.git
cd ssp4sim
cmake --preset=vcpkg -DSSP4SIM_BUILD_TEST=ON
cmake --build build
./build/tests/lib/ssp4sim_tests
```

Or run from python:

```bash
pip install \
  https://github.com/jkCXf9X4/ssp4sim/releases/latest/download/pyssp4sim-0.0.0+lat-cp39-abi3-linux_x86_64.whl
```

Run the same simulation through the CLI wrapper:

```bash
pyssp4sim ./resources/embrace/embrace.json
```

```python
import pyssp4sim

sim = pyssp4sim.Simulator("./resources/embrace/embrace.json")
sim.init()
sim.simulate()
```

## Documentation

- [Overview](docs/overview.md): short feature-oriented summary and positioning.
- [Installation](docs/installation.md): release tarball and wheel consumption.
- [Build from source](docs/build_from_source.md): local and container builds.
- [Usage](docs/usage.md): CLI, Python API, examples, and input data policy.
- [Configuration](docs/configuration.md): JSON keys, defaults, and supported values.
- [Testing](tests/README.md): C++ and Python test layout and commands.
- [Development](docs/development.md): contributor workflow and repository conventions.
- [Release pipeline](docs/linux_binary_distribution.md): Linux binary and wheel packaging.
- [Profiling](docs/profiling.md): build and runtime profiling commands.
- [Logging](docs/logging_guidlines.md): logging levels, hot-path logging, and sink notes.

## Contributing

Contributions are welcome. Development details are in [docs/development.md](docs/development.md).

## License

This project is released under the MIT license. See [LICENSE](LICENSE) for details.
