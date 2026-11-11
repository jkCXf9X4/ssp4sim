---
id: INFO-024
type: info
title: signal module
summary: In-memory simulation data layout and the recorder/export path
date: 2026-11-11
status: current
---

# signal module

Owns in-memory simulation data layout and the recording path; models read and write through SignalStorage, the recorder forwards completed updates to sinks.

- **SignalStorage:** data-centric buffer used by models; stores named variables across timestamped areas backed by RingBuffer.
- **DataRecorder:** connects SignalStorage instances to recorder sinks; copies completed areas into owned buffers, dispatches to sinks via a worker thread.
- **Sinks:** `CsvRecorderSink` rows coalesced by timestamp across storages, the default backend; `SqliteWALRecorderSink` uses WAL journal mode for concurrent readers, per-storage tables `{run_id}_{model}_{storage_name}`, default auto-named `{epoch_seconds}_{session_uuid}.sqlite`, opt-in shared-file mode via `sqlite.file` config — sequential-only.
- **Event path:** model push, storage flags new data, recorder copy, `BoundedMpscEventQueue` (multi-producer single-consumer), sink processing; bounded — 50 areas per storage and a 4096-event queue — with optional backpressure.
- **Ownership:** the recorder holds an explicit copy, no shared memory with the simulation; the data flow is described in `INFO-013`.
- **Include boundary:** `lib/include/signal/` (8 files) plus `lib/include/signal/sinks/` (7 files).
- **Dependencies:** `utils/` (RingBuffer, memory allocation).
