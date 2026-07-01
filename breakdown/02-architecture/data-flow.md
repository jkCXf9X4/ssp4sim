# Data Flow
<!-- Layer: 02-architecture -->
<!-- Stable ID: ARCH-DATAFLOW-001 -->

## Description

This artifact documents the primary data flows through the SSP4SIM engine — from input to execution to recording.

## Flow 1: Configuration to Execution

```
Config JSON → Config::loadFromFile() → dotted key access → FmuHandler, Graph, Executor
```

Configuration drives all runtime behavior: SSP path, executor method, timing, recording format, logging levels.

## Flow 2: SSP Archive to Execution Graph

The SSP is first analyzed to build a structural view, then transformed into an executable graph.

## Flow 3: Simulation Step Data

```
Configuration start_time → Executor.invoke(step_data)
  → FmuModel pulls inputs from SignalStorage
  → FmuModel invokes FMU step
  → FmuModel pushes outputs to SignalStorage
  → SignalStorage emits NewDataEvent
  → DataRecorder copies area into RecorderStorageBuffer
  → NewDataEvent queued in BoundedMpscEventQueue
  → Recorder worker pops event → RecorderSink.on_event()
```

## Flow 4: Recording Output

```
RecorderSink
  → CsvRecorderSink: coalesce by timestamp → write CSV row → flush per interval
  → SqliteWALRecorderSink: write to per-storage table → commit per interval
```

## Data Ownership

- `SignalStorage` owns in-memory simulation data (ring buffers)
- `DataRecorder` owns recorder buffers (copied from SignalStorage)
- No shared mutable state between simulation step and recording (copy handoff)
- Multi-producer single-consumer event queue (BoundedMpscEventQueue) for recorder fan-in

## Traceability

- Backward: Traces to component view in `product-breakdown/02-architecture/component-view.md`.
- Sources: `lib/include/signal/readme.md`, `lib/include/readme.md`.
