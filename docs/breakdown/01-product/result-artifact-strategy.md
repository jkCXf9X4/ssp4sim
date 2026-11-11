---
id: INFO-009
type: info
title: Result Artifact Strategy
summary: The categories of simulation result artifacts SSP4SIM produces and their constraints
date: 2026-11-11
status: current
---

# Result Artifact Strategy

SSP4SIM produces result artifacts in three categories, each serving a distinct access pattern.

- **Local Database (primary):** per-simulation SQLite WAL file for queryable local access, bounded time-window plots, and run-to-run comparison (`REQ-005`).
- **CSV (portable export):** lowest-friction format for scripts, spreadsheets, regression fixtures, and quick inspection (`REQ-004`).
- **Remote Database (future):** central ingestion for multi-source dashboards and fleet-scale comparison; not implemented in current releases.
- **Naming:** SQLite files auto-named `{epoch_seconds}_{session_uuid}.sqlite` with `I{run_id}_{model}_{storage_name}` tables (`REQ-014`).
- **Concurrent-writer safety:** only per-simulation-file SQLite WAL supports parallel execution; CSV and shared-file SQLite are unsafe for concurrent writers (`REQ-005`).
- **Access patterns:** local structured query, portable interchange, and centralized fleet-scale; no single backend serves all three.
