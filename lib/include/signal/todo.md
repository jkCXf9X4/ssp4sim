
But i would like to split the recorder into a main part that can have multiple registered sinks.
As a start the csv sink should still be available

Can you help me reason around this from the current context.

The split between storage and recorder is up for discussion.
I would like to minimize the work wor the storage thread but not at the expense of too much complexity

I have been preparing for the lib/include/signal/recorder.hpp to recieve a callback from the lib/include/signal/storage.hpp when new content becomes available.

## Recorder split design

- `SignalStorage` should stay cheap on the producer path:
  - set `new_data_flags[area]`
  - capture the timestamp
  - call a function-plus-context callback with a small `NewDataEvent`
- `DataRecorder` owns callback registration and assigns recorder-local storage
  indices. Do not use the global `SignalStorage::index` for recorder dispatch.
- `DataRecorder` should not format CSV directly. It should:
  - keep registered `RecorderStorageBuffer` entries
  - enqueue `NewDataEvent` values with recorder-local storage indices
  - drain the queue on its worker thread
  - fan events out to registered `RecorderSink` instances
- Sinks receive raw events on the recorder worker thread, not on the storage
  thread. This keeps storage work low without making every sink solve threading.
- The first sink is `CsvRecorderSink`, preserving current CSV output by
  coalescing timestamp rows internally.

## Implementation progress

- [x] Add `RecorderSink` interface.
- [x] Move CSV file, row buffering, header printing, and row coalescing into
  `CsvRecorderSink`.
- [x] Split `CsvRecorderSink` into separate `csv_recorder_sink.hpp/.cpp` files
  so `recorder.hpp/.cpp` can stay focused on `DataRecorder`.
- [x] Keep `DataRecorder(filename, interval, wait_for)` as the compatibility
  constructor that registers a CSV sink.
- [x] Change storage callbacks to function-plus-context.
- [x] Let `DataRecorder::add_storage()` register callbacks and assign recorder
  storage slots.
- [x] Replace recorder storage polling with an async event queue.
- [x] Update unit tests for CSV compatibility, raw sink dispatch, and source
  storage areas overwritten after callback dispatch.
- [x] Move full-area memcpy into the recorder callback path. Events queued for
  sinks now carry a pointer to a tracker-owned `AlignedBufferPool` snapshot.

## Follow-up thoughts

- `new_data_flags` is still retained for compatibility and diagnostics. The
  recorder now uses the event queue as the synchronization source.
- Event `area` is recorder-local. Event `source_area` preserves the original
  `SignalStorage` ring-buffer area so diagnostics and tests can distinguish the
  source slot from the recorder snapshot slot.
- Source storage areas may be overwritten before the recorder worker processes
  an event. Sinks still see the callback-time value because the recorder copies
  the full storage area into its own snapshot buffer before queueing the event.
