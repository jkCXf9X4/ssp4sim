# Configuration Reference
<!-- Layer: 03-implementation, 05-operation -->


This document describes how `ssp4sim` builds and consumes the input JSON configuration file passed to `sim_app` (or `Simulator(config_path)`).

For CLI and Python invocation examples, see [Usage](usage.md).

## How Configuration Is Built Up

1. The application takes one argument: a path to a JSON config file.
2. `Config::loadFromFile()` reads that file and parses JSON (comments are allowed).
3. Runtime code reads values using dotted keys, for example:
   - `simulation.ssp`
   - `simulation.executor.method`
   - `some_array.0` (array index access)
4. Missing keys are either:
   - rejected (for required keys, using `get*`), or
   - replaced by defaults (for optional keys, using `getOr`).
   - `getOr` does not hide type errors; it only handles missing keys.
5. All string reads support `[TIME]` substitution, replaced with the current timestamp string.
6. `simulation.working_dir` anchors default output locations for the recorder
   and log files when explicit paths are omitted.

## Expected JSON Shape

```json
{
  "simulation": {
    "ssp": "./resources/embrace/embrace_scen.ssp",
    "ssd": "SystemStructure.ssd",
    "start_time": 0.0,
    "stop_time": 10.0,
    "timestep": 0.1,
    "tolerance": 1e-4,
    "realtime": false,
    "working_dir": "./wd/embrace",
    "executor": {
      "method": "jacobi",
      "thread_pool_workers": 5,
      "forward_derivatives": true,
      "sub_step": 0.1,
      "jacobi": { "parallel": true, "method": 1 },
      "seidel": { "parallel": false }
    },
    "recording": {
      "wait_for": false,
      "csv": {
        "enable": true,
        "interval": 0.25
      },
      "duckdb": {
        "enable": true
      },
      "sqlite": {
        "enable": true
      }
    },
    "log": {
      "fmu": false,
      "level_terminal": "info",
      "level_file": "info",
      "level_json": "disable",
      "level_cutelog": "disable"
    }
  }
}
```

## Keys, Types, Defaults, and Values

### `simulation.*`

| Key | Type | Required | Default | Notes |
|---|---|---|---|---|
| `simulation.ssp` | `string` | Yes | - | Path to SSP archive or unpacked SSP directory. |
| `simulation.ssd` | `string` | No | `SystemStructure.ssd` | System structure file inside SSP context. |
| `simulation.start_time` | `double` | Yes | - | Seconds. |
| `simulation.stop_time` | `double` | Yes | - | Seconds. |
| `simulation.timestep` | `double` | Yes | - | Seconds. |
| `simulation.tolerance` | `double` | Yes | - | FMU experiment tolerance. |
| `simulation.realtime` | `bool` | No | `false` | Sync simulation progress to the computer clock. |
| `simulation.working_dir` | `string` | No | `.` | Base directory for default recorder and log output paths. |

### `simulation.executor.*`

| Key | Type | Required | Default | Supported values / behavior |
|---|---|---|---|---|
| `simulation.executor.forward_derivatives` | `bool` | No | `true` | Enables derivative forwarding on model connections. |
| `simulation.executor.method` | `string` | No | `jacobi` | `jacobi`, `seidel`, `custom_delay`, `custom_delay_partial`. |
| `simulation.executor.thread_pool_workers` | `int` | No | `5` | Used by some parallel Jacobi modes. |
| `simulation.executor.sub_step` | `double` | No | `simulation.timestep` | Seconds; used by executors that sub-step. |
| `simulation.executor.jacobi.parallel` | `bool` | No | `false` | If `true`, uses a parallel Jacobi implementation. |
| `simulation.executor.jacobi.method` | `int` | No | `1` | `1` = TBB, `2` = spin pool, `3` = futures. |
| `simulation.executor.seidel.parallel` | `bool` | No | `false` | If `true`, uses `ParallelSeidel`; else `SerialSeidel`. |

Notes:
- Unknown `simulation.executor.method` throws runtime error.
- Unknown `simulation.executor.jacobi.method` throws runtime error.

### `simulation.recording.*`

| Key | Type | Required | Default | Notes |
|---|---|---|---|---|
| `simulation.recording.csv.enable` | `bool` | No | `false` | Enables the CSV export recorder. CSV is the portable export path for the primary local database artifact. |
| `simulation.recording.csv.file` | `string` | No | `simulation.working_dir/result.csv` | Output CSV path (`[TIME]` supported). When omitted, the recorder writes to the working directory. |
| `simulation.recording.csv.interval` | `double` | No | `1.0` | Seconds between recorded CSV samples. |
| `simulation.recording.duckdb.enable` | `bool` | No | `false` | Enables the DuckDB recorder for the primary local database artifact. DuckDB writes grouped storage snapshots directly into a database file, one table per storage. Each table stores `timestamp_ns`, `simulation_time_s`, and the storage variables. The fixed `ssp4sim_metadata` table maps model and storage names to the generated `<model>_<epoch seconds>_<uuid>` data table names. |
| `simulation.recording.duckdb.file` | `string` | No | `simulation.working_dir/result.duckdb` | Output DuckDB database path (`[TIME]` supported). When the file already exists, new run tables and metadata rows are appended. |
| `simulation.recording.sqlite.enable` | `bool` | No | `false` | Enables the SQLite WAL recorder for live-concurrent-reader access. SQLite WAL writes grouped storage snapshots directly into a database file, one table per storage. Each table stores `timestamp_ns`, `simulation_time_s`, and the storage variables. The fixed `ssp4sim_metadata` table maps model and storage names to the generated `<model>_<epoch seconds>_<uuid>` data table names. WAL journal mode allows concurrent readers during writes. The writer batches 10,000 inserted rows per transaction, uses `PRAGMA synchronous=NORMAL`, and defers WAL checkpointing until shutdown to reduce commit overhead; transactions remain atomic, but live readers see data in larger batches and the last committed transactions can be less durable than SQLite `FULL` synchronous mode if the host loses power. |
| `simulation.recording.sqlite.file` | `string` | No | `simulation.working_dir/result.sqlite` | Output SQLite database path (`[TIME]` supported). When the file already exists, new run tables and metadata rows are appended. |
| `simulation.recording.wait_for` | `bool` | No | `false` | If `true`, simulation producer threads wait when recorder buffers are full. If `false`, recorder events can be dropped under backpressure. |
| `simulation.recording.record_inputs` | `bool` | No | `false` | When `true`, input signal values are recorded alongside output signals in CSV and DuckDB artifacts. Input storage has 10 slots; at the default recording interval (0 = every timestep), simulations with more than 10 steps may overflow the input buffer and silently drop events. |

### `simulation.log.*`

| Key | Type | Required | Default | Notes |
|---|---|---|---|---|
| `simulation.log.file` | `string` | No | `simulation.working_dir/sim.log` | Base log file path (`[TIME]` supported). When omitted, log sinks use the working directory. |
| `simulation.log.fmu` | `bool` | No | `false` | Enables FMU-level logging during instantiate/setup/step/terminate. Long multi-line FMU log messages are supported. |
| `simulation.log.level_terminal` | `string` | No | `disable` | Overview tier console sink. Use for immediate human-readable progress while the simulation runs. |
| `simulation.log.level_file` | `string` | No | `disable` | Overview tier plain text file sink. Uses `simulation.log.file` for the durable local summary log. |
| `simulation.log.level_json` | `string` | No | `disable` | Deep-analysis tier JSON file sink. Uses `simulation.log.file + ".json"` for the full structured archive. |
| `simulation.log.level_cutelog` | `string` | No | `disable` | Live navigation tier TCP sink. Uses `127.0.0.1:19996`. `disable` skips the sink. If the endpoint is unavailable, SSP4SIM logs a warning and continues without this sink. |

Logging level values are passed through Quill's `loglevel_from_string()`.
OpenTelemetry is the distributed observability tier and is documented in
[OD-005](../product-breakdown/05-operation/decisions/OD-005.md); it is not yet
part of the current JSON config surface.
Supported values are case-insensitive:

- `tracel3` or `trace_l3`
- `tracel2` or `trace_l2`
- `tracel1` or `trace_l1`
- `debug`
- `info`
- `notice`
- `warning`
- `error`
- `critical`
- `none`

`disable` is an SSP4SIM configuration value, not a Quill log level. Use it to
avoid creating a sink. Any other unknown value throws during simulator
construction.

Logging is initialized after the JSON config is loaded. `Simulator` creates
only the sinks whose `simulation.log.level_*` value is not `disable`, then
constructs project loggers using that sink set. If all sinks are disabled,
logger construction will fail.

The plain file and JSON sinks are independent. Enabling both writes a text log
based on `simulation.log.file` and a JSON log based on
`simulation.log.file + ".json"`. The cutelog sink expects a listener on
`127.0.0.1:19996`; if connection setup fails, only that sink is disabled.

## Validation Behavior

- Missing required keys throw configuration errors before simulation can run.
- Type mismatch (for example string where number is expected) throws type errors.
- JSON comments are accepted in config files.

## Reference Examples

- [`resources/embrace/embrace.json`](../resources/embrace/embrace.json)
- [`resources/generic_config.json`](../resources/generic_config.json)

## Minimal Example

```json
{
  "simulation": {
    "ssp": "./resources/embrace/embrace_scen.ssp",
    "start_time": 0.0,
    "stop_time": 1.0,
    "timestep": 0.01,
    "tolerance": 1e-4,
    "working_dir": "./wd/embrace",
    "recording": {
      "csv": {
        "enable": true
      },
      "duckdb": {
        "enable": true
      },
      "sqlite": {
        "enable": true
      },
      "wait_for": true
    },
    "log": {
      "level_terminal": "info",
      "level_file": "info"
    }
  }
}
```
