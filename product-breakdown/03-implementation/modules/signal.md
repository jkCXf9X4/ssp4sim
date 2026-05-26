# Module: signal
<!-- Layer: 03-implementation -->
<!-- Stable ID: IMP-MOD-SIGNAL-001 -->

## Purpose

Owns the in-memory simulation data layout and the recording path used to export that data. Models read/write through SignalStorage; the recorder observes completed updates and forwards to sinks.

## Key Components

- `SignalStorage`: Data-centric buffer used by models during simulation. Stores named variables across timestamped areas backed by RingBuffer.
- `DataRecorder`: Connects SignalStorage instances to recorder sinks. Copies completed storage areas into owned buffers and dispatches to sinks via worker thread.
- `RecorderStorageBuffer`: Per-storage recorder-side raw data buffer. Decouples recorder processing from source storage.
- `BoundedMpscEventQueue`: Multi-producer single-consumer queue for recorder events.
- `CsvRecorderSink`: Writes registered storage variables to CSV with timestamp coalescing.
- `DuckDBRecorderSink`: Writes to DuckDB database with per-storage tables.

## Sinks

- CSV sink: Row-oriented, coalesces by timestamp across storages. Default recording backend.
- DuckDB sink: Column-oriented database with per-storage tables. Optional.

## Include Boundary

- Path: `lib/include/signal/` (8 files) + `lib/include/signal/sinks/` (7 files)

## Dependencies

- `utils/` — for RingBuffer, memory allocation

## Notable Patterns

- Data path: model push → storage flag_new_data → recorder copy → queue → sink process
- Bounded buffers (50 per storage, 4096 event queue) with optional backpressure
- Recorder stores explicit ownership copy (no shared memory with simulation)
- Coalescing CSV rows by timestamp across storages

## Traceability

- Backward: Architecture component view, data flow view.
- Sources: `lib/include/signal/readme.md`, `lib/include/signal/`.