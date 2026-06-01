# Usage
<!-- Layer: 05-operation, 01-product -->


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

## Result Artifacts

SSP4SIM treats simulation results as artifacts that should be useful at three
access levels:

| Category | Current support | Main use case |
|---|---|---|
| Local database | Primary result artifact category. Supported today through DuckDB; SQLite WAL is the preferred future local-live database shape | Queryable local storage for viewers, bounded time-window plots, and run-to-run comparison without repeatedly parsing CSV. |
| CSV | Portable export path via `simulation.recording.csv.*` | Lowest-friction access for scripts, spreadsheets, regression fixtures, and quick inspection. |
| Remote database | Not implemented; the remote ingest interface is decided as InfluxDB Line Protocol ([OD-004](../product-breakdown/05-operation/decisions/OD-004.md)) | Central ingestion from multiple simulation sources for shared dashboards, fleet-scale comparison, and long-lived history. |

The local database is the primary result artifact because it gives the viewer a
typed, indexed query layer for recent windows and run-to-run comparison.
CSV remains the lowest-friction export for scripts and archival handoff. The
current local database backend is DuckDB, which is good for post-run analysis
and table export. For a separate live writer and viewer process, SQLite in WAL
mode is a better fit than DuckDB because it supports one writer with concurrent
readers in a single local file. Remote database output remains outside the
current configuration surface.

Recording details live in [Configuration](configuration.md).

To unpack a DuckDB result into per-table CSV files, use:

```bash
./venv/bin/python resources/scripts/export_duckdb_tables.py \
  wd/signal_sine_gain_add/baseline/result.duckdb
```

By default the script writes CSVs to a sibling directory named
`result_csv/` next to the database file.

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

## Result Artifact Policy

The result artifact policy and supporting tradeoffs are documented in
[PD-001](../product-breakdown/01-product/decisions/PD-001.md) and the
[capabilities page](../product-breakdown/01-product/capabilities.md).
