# Configuration Reference

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
    "executor": {
      "method": "jacobi",
      "thread_pool_workers": 5,
      "forward_derivatives": true,
      "sub_step": 0.1,
      "jacobi": { "parallel": true, "method": 1 },
      "seidel": { "parallel": false }
    },
    "recording": {
      "enable": true,
      "wait_for": false,
      "interval": 0.25,
      "result_file": "./results/sim_[TIME].csv"
    },
    "log": {
      "file": "./results/sim_[TIME].log",
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
| `simulation.recording.enable` | `bool` | No | `true` | Enables CSV recorder creation. |
| `simulation.recording.wait_for` | `bool` | No | `false` | If `true`, simulation producer threads wait when recorder buffers are full. If `false`, recorder events can be dropped under backpressure. |
| `simulation.recording.interval` | `double` | No | `1.0` | Seconds between recorded samples. |
| `simulation.recording.result_file` | `string` | No | `./result/data.csv` | Output CSV path (`[TIME]` supported) |

### `simulation.log.*`

| Key | Type | Required | Default | Notes |
|---|---|---|---|---|
| `simulation.log.file` | `string` | Yes | - | Base log file path (`[TIME]` supported). Required before sink selection is evaluated. |
| `simulation.log.fmu` | `bool` | No | `false` | Enables FMU-level logging during instantiate/setup/step/terminate. Long multi-line FMU log messages are supported. |
| `simulation.log.level_terminal` | `string` | No | `disable` | Console sink level. `disable` skips the sink. |
| `simulation.log.level_file` | `string` | No | `disable` | Plain text file sink level. Uses `simulation.log.file`. `disable` skips the sink. |
| `simulation.log.level_json` | `string` | No | `disable` | JSON file sink level. Uses `simulation.log.file + ".json"`. `disable` skips the sink. |
| `simulation.log.level_cutelog` | `string` | No | `disable` | Cutelog TCP sink level. Uses `127.0.0.1:19996`. `disable` skips the sink. |

Logging level values are passed through Quill's `loglevel_from_string()`.
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
`simulation.log.file + ".json"`. The cutelog sink expects a listener to already
be available on `127.0.0.1:19996`.

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
    "recording": {
      "enable": true,
      "wait_for": true,
      "result_file": "./results/embrace.csv"
    },
    "log": {
      "file": "./results/embrace.log",
      "level_terminal": "info",
      "level_file": "info"
    }
  }
}
```
