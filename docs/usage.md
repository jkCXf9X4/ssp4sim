# Usage

This page covers running simulations after SSP4SIM is installed or built. For
installation steps, see [Installation](installation.md). For source builds, see
[Build From Source](build_from_source.md).

## CLI

Run `sim_app` with one JSON configuration file:

```bash
sim_app ./resources/embrace/embrace.json
```

From a source checkout, use the built executable:

```bash
./build/public/ssp4sim_app/sim_app ./resources/embrace/embrace.json
```

Show the embedded CLI version:

```bash
sim_app --version
```

Simulation input is controlled by the JSON file passed to `sim_app`. The full
configuration key reference is in [Configuration](configuration.md).

Example configs:

- [`resources/embrace/embrace.json`](../resources/embrace/embrace.json)
- [`resources/generic_config.json`](../resources/generic_config.json)

Recording output defaults to CSV. If `simulation.recording.influx.enable` is
set, SSP4SIM also writes one InfluxDB point per signal update using the
configured base URL, database name, measurement name, run tag, and optional
auth token. For local runs, `SSP4SIM_INFLUX_TOKEN` can supply the token when it
is not set in the config file.

## Python API

Install the release wheel or build the Python API from source before importing
`pyssp4sim`.

```python
import pyssp4sim

sim = pyssp4sim.Simulator("./resources/embrace/embrace.json")
sim.init()
sim.simulate()
```

The wheel also installs a `pyssp4sim` command that wraps the same flow:

```bash
pyssp4sim ./resources/embrace/embrace.json
```

## Input Data Policy

SSP4SIM records simulation results to CSV, but CSV is not treated as the
preferred way to define portable simulation inputs. CSV input conventions vary
between tools and are difficult to reproduce across simulation engines without
extra metadata or standardization.

Use a portable FMU to supply input variables when repeatability matters. That
keeps input behavior, derivatives, and signal continuity inside a model that can
travel with the SSP.
