---
id: INFO-017
type: info
title: Configuration
summary: End-to-end configuration pipeline from JSON file to runtime behavior
date: 2026-11-11
status: current
---

# Configuration

One JSON config file drives all runtime behavior; the full key table, type defaults, and validation are runnable reference, not this leaf.

- **Pipeline:** the application receives one argument, the config path; `Config::loadFromFile()` parses the JSON (comments allowed via nlohmann_json); runtime modules read values by dotted key.
- **Required keys:** `simulation.ssp` (SSP path), timing (`start_time`, `stop_time`, `timestep`), `simulation.executor.method` (jacobi/seidel); a missing required key is rejected with a type-checked error via `get*`.
- **Optional keys:** `simulation.executor.thread_pool_workers`, `simulation.recording.csv.enable` (default true), `simulation.log.level_terminal` and `simulation.log.level_file`; a missing optional key takes its default via `getOr(key, default)`.
- **Substitutions:** every string read replaces `[TIME]` with the current timestamp; `simulation.working_dir` anchors default output locations for recorder and log files.
- **Schema reference:** full key table, type defaults, and validation behavior are documented in `docs/configuration.md`.
