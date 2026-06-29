# Result Artifact Strategy
<!-- Layer: 01-product -->
<!-- Stable ID: PRO-ARTIFACT-001 -->

## Description

This artifact describes the result artifact strategy — the categories of simulation output artifacts SSP4SIM produces, their intended use, and their constraints.

## Artifact Categories

### 1. Local Database (Primary)

- **Backend**: SQLite WAL (per-simulation-file)
- **Access**: Queryable local storage for viewers, bounded time-window plots, run-to-run comparison
- **Concurrent-writer safe**: Yes (per-sim files only)
- **Configuration**: `simulation.recording.sqlite.*`
- **Naming convention**: `{epoch_seconds}_{session_uuid}.sqlite` with `I{run_id}_{model}_{storage_name}` tables
- **Use Case Traceability**: Serves **PRO-UC-005** (Query Local Result Database) — provides the queryable per-sim SQLite store that the viewer and comparison tool consume. Does not serve PRO-UC-006 directly because local per-sim files do not provide central ingestion.

### 2. CSV (Portable Export)

- **Backend**: Standard CSV output
- **Access**: Lowest-friction for scripts, spreadsheets, regression fixtures, quick inspection
- **Concurrent-writer safe**: No (file-level append race)
- **Configuration**: `simulation.recording.csv.*`
- **Use Case Traceability**: Provides a portable export path for use cases that need scriptable access (supplementary to PRO-UC-005). Not suitable for PRO-UC-006 without additional aggregation tooling.

### 3. Remote Database (Future)

- **Remote interface**: InfluxDB Line Protocol (OD-004)
- **Access**: Central ingestion for multi-source dashboards, fleet-scale comparison
- **Status**: Not implemented — deferred per OD-002
- **Use Case Traceability**: When implemented, serves **PRO-UC-006** (Ingest Results into Remote Database) — this is the primary solution for central ingestion, dashboards, and multi-source comparison.

## Design Rationale

### SQLite WAL (Primary)

SQLite WAL is the primary/default backend because it provides queryable structured access with concurrent-writer safety via per-sim files, which is the core use case for interactive post-hoc analysis. The tradeoff is full database overhead in exchange for structured queryability.

### CSV (Portable Export)

CSV exists alongside SQLite as the lowest-friction interchange format for scripts, spreadsheets, regression fixtures, and users who do not want or need database tooling. The cost is no concurrent-writer safety and no queryability without external tooling.

### Remote Database (Future)

Remote database support is deferred rather than built now because multi-source central ingestion requires distributed infrastructure and consensus on wire protocol (InfluxDB Line Protocol per OD-004). It is a separate category rather than just an additional sink on the local DB because the deployment model, access patterns, and lifecycle differ fundamentally — central shared vs. local isolated.

### Why Three Categories

No single backend serves all access patterns: local structured query, portable interchange, and centralized fleet-scale. This is a classic artifact distribution driven by consumption context, not by implementation convenience.

## Concurrent-Writer Safety

| Backend | Concurrent-writer safe? | Reason |
|---|---|---|
| SQLite WAL (per-sim files, default when `sqlite.file` absent) | **Yes** | Each simulation gets an independent `.sqlite` file. No write-lock contention. |
| SQLite WAL (single shared file, opt-in via `sqlite.file`) | **No** | SQLite write lock is per database file, not per table. Writers serialize. Must never be used with concurrent writers. |
| CSV | **No** | File-level append race. Concurrent writes produce corrupted output. |

## Traceability

- Backward: PD-001, CAP-004, CAP-005, domain-model.md (Recording), PRO-UC-005, PRO-UC-006, OD-002, OD-004
- Sources: `docs/usage.md` (Result Artifacts section), `docs/configuration.md` (recording section)
- Per-category traceability: Documented inline in the Artifact Categories section above
