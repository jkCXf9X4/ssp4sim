
# Signal handling

The signal module owns the in-memory simulation data layout and the recording
path used to export that data. Models read and write values through
`SignalStorage`; the recorder observes completed storage updates and forwards
stable snapshots to one or more recorder sinks.

## Module overview

### `storage.hpp` / `storage.cpp`

`SignalStorage` is the data-centric buffer used by models during simulation.
It stores a fixed set of named variables across a fixed number of time-stamped
areas backed by `utils::RingBuffer`.

Each variable is described by `SignalInfo`, which contains the variable name,
data type, aligned byte position in each storage area, and optional derivative
layout. After variables are added, `allocate()` creates the backing ring buffer
and resolves fast absolute pointers for each variable and derivative in every
area.

The normal data path is:

1. A model obtains or creates an area for a simulation timestamp with
   `push()` or `get_or_push()`.
2. The model reads or writes variable memory through `get_item()` and optional
   derivative memory through `get_derivative()`.
3. Once the area contains a complete update, `flag_new_data()` emits a
   `NewDataEvent` through the registered callback.

`NewDataEvent` is the handoff object between storage and recording. Storage
populates the source storage pointer, source area, and timestamp. The recorder
later fills in the recorder-owned buffer pointer and recorder storage index.

### `recorder.hpp` / `recorder.cpp`

`DataRecorder` connects one or more `SignalStorage` instances to recorder
sinks. It registers itself as each storage callback, copies new storage areas
into recorder-owned buffers, queues events, and runs a worker thread that
delivers events to sinks. `start_recording()` calls an optional
`RecorderSink::start()` hook before the worker thread begins so sinks can
capture run-specific state.

`DataRecorder::add_storage()` creates a `RecorderStorageBuffer` for the
storage, records the storage-to-buffer index mapping, installs the storage
callback, and notifies every sink that a storage was added. `init()` then lets
sinks prepare their output layouts before `start_recording()` starts the worker
thread.

The default constructor adds a `CsvRecorderSink`, so the common use case is a
single CSV output file. Additional sinks can be added through `add_sink()` when
another export path needs the same event stream.

### `record_tracker.hpp`

`RecorderStorageBuffer` owns the recorder-side raw data buffer for one
`SignalStorage`. Its purpose is to decouple recorder processing from the source
storage ring buffer: when storage reports new data, the recorder copies the
source area into this buffer before the model can overwrite the source area.

Each `SignalStorage` has one producer, so `RecorderStorageBuffer` only tracks
recorder lag for that storage. The shared event queue handles fan-in across
multiple storages. `active_items` counts buffered events that have not yet been
released by the recorder worker.

### `mpsc_event_queue.hpp`

`BoundedMpscEventQueue<T>` is the bounded multi-producer, single-consumer queue
used by `DataRecorder`. Storage callback threads act as producers and the
recorder worker thread is the only consumer.

The queue uses per-cell sequence numbers, an atomic enqueue position, and a
single consumer dequeue position. `try_push()` returns `false` when the queue is
full; `try_pop()` returns `false` when no event is ready.

### `csv_recorder_sink.hpp` / `csv_recorder_sink.cpp`

`CsvRecorderSink` is a `RecorderSink` implementation that writes registered
storage variables to CSV. When a storage is added, it records each variable's
name, type, byte position, and output column. During `init()` it prints the CSV
header and prepares a fixed row buffer.

On each event, the sink uses the stored byte offsets to parse the recorder-owned
event buffer. Updates from different storages with the same timestamp are
coalesced into the same CSV row. Rows are written when the row buffer rolls over
and again during `stop()`, which flushes and closes the file.

### `influx_recorder_sink.hpp` / `influx_recorder_sink.cpp`

`InfluxRecorderSink` mirrors the recorder event stream to InfluxDB. Sink configuration and behavior are documented in [Configuration Reference](../../../docs/configuration.md#influx-recording).

The sink is safe to use without a running InfluxDB service. Connection or write
failures are caught, logged, and used to disable only the Influx sink so the
simulation can continue.

## Runtime interaction

The modules interact in this order:

1. Simulation setup creates one or more `SignalStorage` objects, adds variables,
   and calls `allocate()`.
2. A `DataRecorder` is created and each storage is registered with
   `add_storage()`.
3. The recorder creates one `RecorderStorageBuffer` per storage and notifies
   recorder sinks so they can map storage layouts.
4. `DataRecorder::init()` initializes all sinks.
5. `start_recording()` calls `RecorderSink::start()` on each sink, then starts
   the recorder worker thread.
6. Models update storage areas and call `flag_new_data()`.
7. The storage callback enters `DataRecorder::enqueue_event()`.
8. The recorder copies the source storage area into the matching
   `RecorderStorageBuffer`, attaches that raw buffer to the event, pushes the
   event into `BoundedMpscEventQueue`, and wakes the worker through
   `event_signal`.
9. The worker thread pops queued events and calls `RecorderSink::on_event()` for
   every registered sink.
10. After sink processing, the worker releases the recorder buffer slot by
    calling `RecorderStorageBuffer::pop()`.
11. `stop_recording()` stops the worker and calls `RecorderSink::stop()` so
    sinks can flush pending output.

## Buffering and backpressure

Recorder events are intentionally copied into recorder-owned buffers before
they are queued. This keeps sinks from reading directly from `SignalStorage`
areas that may be reused by the simulation.

The recording path has two bounded resources:

- The per-storage `RecorderStorageBuffer`, currently created with capacity 50.
- The shared `BoundedMpscEventQueue<NewDataEvent>`, currently created with
  capacity 4096.

If either resource is full and `wait_for_recorder` is disabled, the recorder
drops the event and logs a rate-limited warning. If `wait_for_recorder` is
enabled, producers spin/yield until capacity is available or recording stops.
