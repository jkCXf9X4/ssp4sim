# SSP4SIM

SSP4SIM is a C++23 library and application for simulating Structure and Parameterization (SSP) archives. The goal is to create a small experimental simulation engine to develop and test novel (and old) simulation strategies.

It currently supports the following execution strategies:
- Gauss-Jacobi
- Gauss-Seidel 

It currently supports SSP1.0 and FMI2.0 models.

See the [SSP standard](https://ssp-standard.org) for more information about the file format.

Builds upon the [SSP4CPP](https://github.com/jkCXf9X4/ssp4cpp) XML deserializer

## Install (Linux Release Binaries)
Tagged releases (`v*`) publish dual artifacts:
- Linux bundle: `ssp4sim-linux-x86_64-vX.Y.Z.tar.gz`
- Python wheel: `pyssp4sim-X.Y.Z-*.whl`

The Linux tarball contains:
- `bin/sim_app`
- `python/pyssp4sim` (Python API package with native extension)
- `readme.md`, `LICENSE`, `version.txt`, `RELEASE.txt`

Download and verify:
https://github.com/jkCXf9X4/ssp4sim/releases/latest

```bash
curl -L -o ssp4sim-linux.tar.gz \
  https://github.com/jkCXf9X4/ssp4sim/releases/latest/download/ssp4sim-linux-x86_64-vX.Y.Z.tar.gz

curl -L -o SHA256SUMS \
  https://github.com/jkCXf9X4/ssp4sim/releases/latest/download/SHA256SUMS

sha256sum -c SHA256SUMS

mkdir -p $HOME/.local/opt/ssp4sim
tar -xzf ssp4sim-linux.tar.gz -C $HOME/.local/opt
```

Add to PATH:
```bash
export PATH="$HOME/.local/opt/ssp4sim/bin:$PATH"
```

## Run A Simulation

Run the CLI app with a JSON configuration file:

```bash
sim_app ./resources/embrace/embrace.json
```

or, when running from a source checkout:

```bash
./build/public/ssp4sim_app/sim_app ./resources/embrace/embrace.json
```

Runtime notes:
- Releases are currently built on Ubuntu 22.04.
- The binary links `libstdc++` statically, but system compatibility can still depend on glibc and other runtime components.

## Configuration
Simulation input is controlled via a JSON file passed to `sim_app` as its single argument.

- Full key reference and supported values: `docs/configuration.md`

- Example config: `resources/embrace/embrace.json`
- Example config: `resources/f16/config.json`
- Example config: `resources/generic_config.json`
- Example config set: `resources/delay_sys/*.json`

## Python API

Install Python API from a release wheel:

```bash
pip install ./pyssp4sim-X.Y.Z-*.whl

pip install https://github.com/jkCXf9X4/ssp4sim/releases/download/vX.Y.Z/pyssp4sim-0.1.0-cp311-cp311-linux_x86_64.whl
```

Usage:

```python
import py_ssp4sim

sim = py_ssp4sim.Simulator("./resources/embrace/embrace.json")
sim.init()
sim.simulate()
```

## Documentation

- Build from source: `docs/build_from_source.md`
- Development workflows: `docs/development.md`
- Configuration reference: `docs/configuration.md`
- Linux release pipeline: `docs/linux_binary_distribution.md`
- Profiling: `docs/profiling.md`
- Logging guidelines: `docs/logging_guidlines.md`

## Contributing
Contributions are welcome. Development details are in `docs/development.md`.

## License
This project is released under the MIT license. See [LICENSE](LICENSE) for details.
