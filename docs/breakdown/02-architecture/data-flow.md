---
id: INFO-013
type: info
title: Data flow
summary: Primary runtime data flows from configuration through execution to recording
date: 2026-11-11
status: current
---

# Data flow

Four primary flows carry data from configuration to execution graph to recorded output.

- **Configuration to execution:** JSON config file, `Config::loadFromFile()`, dotted-key access, then FmuHandler, Graph, and executor. Configuration drives SSP path, executor method, timing, recording format, and logging levels; the config pipeline is implementation (`INFO-017`).
- **SSP archive to execution graph:** the SSP is analyzed into a structural view, then transformed into an executable dependency graph (component view `INFO-012`).
- **Simulation step data:** step_data invocation on the executor; model pulls inputs from SignalStorage; FMU step invoked; model pushes outputs back; SignalStorage emits NewDataEvent; recorder copies the area into RecorderStorageBuffer; event queued; recorder worker pops it and forwards to sinks.
- **Recording output:** CSV sink coalesces by timestamp and flushes per interval; SQLite WAL sink writes per-storage tables and commits per interval; recorder internals are the signal module (`INFO-024`).
- **Data ownership:** SignalStorage owns in-memory simulation data (ring buffers); the recorder owns copied buffers; no shared mutable state between step and recording; a multi-producer single-consumer event queue handles recorder fan-in.
