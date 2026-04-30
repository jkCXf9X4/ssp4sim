
# Signal handling

The models operate on the SignalStorage, this is where the data that is used within the simulations is used

The recorder thread copies each `SignalStorage` update into a recorder-owned
raw event buffer before the source storage area can be overwritten.
Recorder events are published through a bounded lock-free MPSC queue and a
monotonic atomic wake signal so producer threads do not take the recorder
worker mutex on the hot path. Each `SignalStorage` has one producer; the shared
event queue handles fan-in from multiple storages to the single recorder worker.
When `wait_for_recorder` is enabled, producers spin/yield while recorder-owned
buffers or the event queue are full instead of dropping the event.

Recorder sinks consume those raw `NewDataEvent` blocks. `CsvRecorderSink`
maps storage variable offsets when storage is registered, parses event buffers
by that layout, coalesces same-timestamp storage updates into one row, and
writes the CSV file on stop or row-buffer rollover.
