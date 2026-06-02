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
and table export. For concurrent-simulation workflows with a live viewer,
SQLite WAL with per-simulation files is the correct choice because each
simulation gets an independent file, enabling parallel writes without lock
contention. Remote database output remains outside the current configuration
surface.

## SQLite Naming Convention

When `simulation.recording.sqlite.file` is not set, the SQLite WAL sink
auto-generates one database file per simulation run:

- **Filename**: `{epoch_seconds}_{session_uuid}.sqlite`
  - `epoch_seconds`: Unix epoch timestamp at database open time.
  - `session_uuid`: RFC 4122 UUIDv4 generated once per `Simulation` lifetime.
- **Table names**: `I{run_id}_{model}_{storage_name}`
  - `run_id`: auto-incrementing counter persisted in the `ssp4sim_run_counter` table. Starts at 1 for fresh files, increments for appends to shared files.
  - `model` and `storage_name`: sanitized alphanumeric components from the signal storage name.
- **Example**: `1748739201_a1b2c3d4-e5f6-4789-abcd-ef0123456789.sqlite` containing tables like `I1_Consumer_output`, `I1_Aux_output`.

When `sqlite.file` is set explicitly (shared-file mode), the run counter persists
across runs: run 1 gets `I1_Consumer_output`, run 2 gets `I2_Consumer_output`, etc.
The `ssp4sim_run_counter` table enables table discovery without a metadata side-table.

## Concurrent Execution and Local Database Safety

**No local output format supports safe concurrent multi-writer access except
per-simulation-file SQLite WAL.** This is a fundamental constraint, not a
transient limitation:

| Backend | Concurrent-writer safe? | Reason |
|---|---|---|
| SQLite WAL (per-sim files, default when `sqlite.file` absent) | **Yes** | Each simulation gets an independent `.sqlite` file. No write-lock contention. |
| SQLite WAL (single shared file, opt-in via `sqlite.file`) | **No** | SQLite write lock is per database file, not per table. Writers serialize. Must never be used with concurrent writers. |
| DuckDB | **No** | Single-writer mode. Does not support concurrent writers at all. |
| CSV | **No** | File-level append race. Concurrent writes produce corrupted output. |

If you need to run multiple simulations concurrently (e.g., parameter sweeps,
Monte Carlo runs), you **must** use the SQLite WAL sink without setting
`simulation.recording.sqlite.file` — the per-simulation auto-naming is the
safe default. If you set `sqlite.file` to use a shared file, the concurrent
safety guarantee is lost and the runs must be strictly sequential. CSV and
DuckDB sinks can run alongside SQLite for post-run analysis but must not be
the sole recorder in concurrent workflows.

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
