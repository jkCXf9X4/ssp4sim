# Configuration
<!-- Layer: 03-implementation -->
<!-- Stable ID: IMP-CONFIG-001 -->

## Description

This artifact describes the end-to-end configuration pipeline — from JSON file to runtime behavior.

## Pipeline

```
JSON config file → Config::loadFromFile() → dotted key access → runtime setup
```

## Config Flow

1. Application receives one argument: path to a JSON config file
2. `Config::loadFromFile()` reads and parses JSON (comments allowed via nlohmann_json)
3. Runtime modules read values using dotted keys:
   - `simulation.ssp` → SSP archive path
   - `simulation.executor.method` → executor selection (jacobi/seidel)
   - `simulation.recording.csv.enable` → CSV recording toggle
   - `simulation.log.level_terminal` → terminal log level
4. Missing keys:
   - `get*(key)` — rejected with type-checked error (required keys)
   - `getOr(key, default)` — replaced by default (optional keys)
5. `[TIME]` substitution — all string reads replace `[TIME]` with current timestamp string
6. `simulation.working_dir` anchors default output locations for recorder and log files

## Schema Reference

Full JSON schema is documented in `docs/configuration.md`. Key sections:

- `simulation.ssp`: SSP path (required)
- `simulation.start_time` / `simulation.stop_time` / `simulation.timestep`: Timing (required)
- `simulation.executor.method`: "jacobi" or "seidel" (required)
- `simulation.executor.thread_pool_workers`: Thread count (optional, default varies)
- `simulation.recording.csv.enable`: CSV toggle (optional, default true)
- `simulation.recording.duckdb.enable`: DuckDB toggle (optional, default false)
- `simulation.log.level_terminal` / `simulation.log.level_file`: Log levels (optional)

## Traceability

- Backward: Product requirements (REQ-001 through REQ-006 drive config keys).
- Sources: `docs/configuration.md`, `lib/include/utils/config/`.