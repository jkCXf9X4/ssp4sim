# SSP4SIM

SSP4SIM is a C++23 library and application for simulating Structure and Parameterization (SSP) archives. The goal is to create a small experimental simulation engine to develop and test novel (and old) simulation strategies.

It currently supports the following execution strategies:
- Gauss-Jacobi
- Gauss-Seidel 

It currently supports SSP1.0 and FMI2.0 models.

See the [SSP standard](https://ssp-standard.org) for more information about the file format.

Builds upon the [SSP4CPP](https://github.com/jkCXf9X4/ssp4cpp) XML deserializer
and uses [Quill](https://github.com/odygrd/quill) for logging.

## Installation

Installation workflows are documented in [docs/installation.md](docs/installation.md).

That guide covers:

- Linux release binaries
- Python wheel installation
- Source-build prerequisites and clone setup

## Run A Simulation

Run the CLI app with a JSON configuration file:

```bash
sim_app ./resources/embrace/embrace.json
```

or, when running from a source checkout:

```bash
./build/public/ssp4sim_app/sim_app ./resources/embrace/embrace.json
```

Show the embedded CLI version:

```bash
sim_app --version
```

## Configuration
Simulation input is controlled via a JSON file passed to `sim_app` as its single positional argument.

- Full key reference and supported values: [docs/configuration.md](docs/configuration.md)

- Example config: [resources/embrace/embrace.json](resources/embrace/embrace.json)
- Example config: [resources/f16/config.json](resources/f16/config.json)
- Example config: [resources/generic_config.json](resources/generic_config.json)
- Example config set: [resources/delay_sys/](resources/delay_sys/)

## Python API

Installation details are in [docs/installation.md](docs/installation.md).

Usage:

```python
import pyssp4sim

sim = pyssp4sim.Simulator("./resources/embrace/embrace.json")
sim.init()
sim.simulate()
```

## This will not support csv or other non tool repeatable sulutions
Since each tool implemets its own solution, its not repeatable between simulaiton engines without sufficient metadata or standardization

A portable fmu should be used to supply the input variables.
This enables:
 - portabillity
 - repeatabillity
 - controlled derivatives
 - signal derivative continuity


## Documentation

- [Installation guide](docs/installation.md)
- [Build from source](docs/build_from_source.md)
- [Development workflows](docs/development.md)
- [Configuration reference](docs/configuration.md)
- [Linux release pipeline](docs/linux_binary_distribution.md)
- [Profiling](docs/profiling.md)
- [Logging guidelines](docs/logging_guidlines.md)

## Contributing
Contributions are welcome. Development details are in [docs/development.md](docs/development.md).

## License
This project is released under the MIT license. See [LICENSE](LICENSE) for details.
