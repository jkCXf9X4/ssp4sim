
# Signal handling

The models operate on the SignalStorage, this is where the data that is used within the simulations is used

The recorder thread copies each `SignalStorage` update into a recorder-owned
raw event buffer before the source storage area can be overwritten.

Recorder sinks consume those raw `NewDataEvent` blocks. `CsvRecorderSink`
maps storage variable offsets when storage is registered, parses event buffers
by that layout, coalesces same-timestamp storage updates into one row, and
writes the CSV file on stop or row-buffer rollover.
