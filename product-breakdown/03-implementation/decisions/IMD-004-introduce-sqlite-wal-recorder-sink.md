# IMD-004: Introduce SQLite WAL Recorder Sink
<!-- Layer: 03-implementation -->
<!-- Status: Accepted -->

## Context

The local database recording layer currently supports DuckDB and CSV sinks.
DuckDB provides excellent analytical capabilities but is designed around
columnar batch writes and a single-writer, single-reader operational model.
This makes it less suitable for live one-writer, many-reader viewer workflows
where a separate viewer process reads the database file while the simulation
is still writing.

A SQLite sink with WAL journal mode offers:

- Concurrent reads during ongoing writes (WAL mode)
- Simple, low-overhead prepared statement interface
- Ubiquitous tooling and library support
- No server process needed

## Decision

Add `SqliteWALRecorderSink`, a new recorder sink that writes simulation data
into a SQLite database file with WAL journal mode enabled.

## Rationale

- **WAL concurrency**: SQLite WAL allows one writer and multiple simultaneous
  readers, enabling a live viewer to open and query the database file while
  the simulation is still running.
- **Transaction strategy**: Wrapping inserts in explicit transactions and
  committing every 100 rows balances write durability with batch efficiency.
  Each event batch can trigger a commit without excessive fsync overhead.
- **Type mapping**: FMI 2.0 types map to simple SQLite storage classes:
  `REAL` → `REAL`, `INTEGER`/`ENUMERATION`/`BOOLEAN` → `INTEGER`,
  `STRING` → `TEXT`. BOOLEAN is stored as INTEGER (0/1) since SQLite has no
  native boolean type.
- **No WAL checkpoint exposure**: The default SQLite WAL checkpoint behavior
  is retained. Future work could expose checkpoint control for very long runs.
- **Don't share DuckDB utils**: SQLite API surface (C callbacks, `sqlite3_stmt`
  handles) is entirely different from DuckDB's C API. Sharing or refactoring
  would introduce unnecessary abstraction. Each sink owns its utils namespace.

## Consequences

- New build dependency: `unofficial-sqlite3` via vcpkg.
- New config keys: `simulation.recording.sqlite.enable`, `simulation.recording.sqlite.file`.
- Metadata table `ssp4sim_metadata` mirrors the DuckDB pattern for cross-sink
  consistency of run metadata.
- Table naming follows the same `<model>_<epoch>_<uuid>` convention as DuckDB.
- Default file is `result.sqlite` in the working directory.
- Event-per-row recording (same event detail as DuckDB, no interval coalescing).

## Affected Artifacts

- `vcpkg.json` — add `sqlite3` dependency
- `lib/CMakeLists.txt` — add find_package and link for unofficial-sqlite3
- `lib/public_include/shared_config.hpp` — add `SqliteRecordingConfig`
- `lib/include/signal/sinks/sqlite_recorder_storage.hpp` — new storage layout types
- `lib/include/signal/sinks/sqlite_recorder_utils.hpp` — new utility declarations
- `lib/include/signal/sinks/sqlite_recorder_utils.cpp` — new utility implementation
- `lib/include/signal/sinks/sqlite_recorder_sink.hpp` — new sink class header
- `lib/include/signal/sinks/sqlite_recorder_sink.cpp` — new sink class implementation
- `lib/include/simulation.cpp` — register the sink
- `tests/lib/core/test_sqlite_recorder.cpp` — test coverage
- `docs/configuration.md` — document SQLite config keys
- `product-breakdown/01-product/domain-model.md` — update Recording description
- `product-breakdown/03-implementation/modules/signal.md` — add sink to Key Components and Sinks
- `tests/README.md` — add test file to docs
- `product-breakdown/06-evolution/backlog/candidates/IMP-029.md` → done

## Traceability

- Backward: IMP-029 SQLite WAL local database sink.
- Sources: `lib/include/signal/sinks/sqlite_recorder_sink.hpp`,
  `lib/include/signal/sinks/sqlite_recorder_utils.hpp`,
  `lib/include/signal/sinks/sqlite_recorder_storage.hpp`.