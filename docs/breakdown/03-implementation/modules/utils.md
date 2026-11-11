---
id: INFO-026
type: info
title: utils module
summary: Shared low-level helpers used by every other module
date: 2026-11-11
status: current
---

# utils module

Shared helpers used across all modules; the foundation layer with zero internal dependencies.

- **Components:** `Config` (JSON loading, dotted-key access, required/optional reads, `[TIME]` substitution, array index access like `some_array.0`), `RingBuffer` (fixed capacity set at allocation), `TaskThreadPool` (two variants), `Tarjan` (strongly-connected-component detection), `Allocator`, `Time`, `Timer`, `Node`/`NodeIterator`, `Map`/`Vector`.
- **Role:** lowest-level module — no internal module dependencies; every other module may depend on it.
- **Dependencies:** nlohmann_json for config parsing.
- **Include boundary:** `lib/include/utils/`, 26 files.
- **Uses:** RingBuffer backs SignalStorage areas (`INFO-024`).
- **Note:** two thread-pool implementations exist; the newer variant may replace the older one.
