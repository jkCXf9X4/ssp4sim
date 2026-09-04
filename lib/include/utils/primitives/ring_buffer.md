# `ssp4sim::utils::RingBuffer`

A small, fixed-capacity ring (circular) buffer that stores fixed-size byte
records. When full, new writes **continuously overwrite the oldest** entry.
Each slot also carries a `uint64_t` timestamp used for time-based lookups and
for ordering. It backs `SignalStorage` (see
[`signal_storage.md`](../../simulation/signal/signal_storage.md)) and is also
used internally by the recorder for back-pressure buffering.

- Namespace: `ssp4sim::utils`
- Header: `lib/include/utils/primitives/ring_buffer.hpp`
- Implements: `types::IWritable` (i.e. supplies `to_string()`).

## General design

- **Fixed slots.** The constructor takes `capacity` (number of slots) and
  `buffer_size` (bytes per slot). All backing memory is allocated up front.
- **Head-tracking.** `head` points at the most recently written slot.
  `write_count` counts total writes since construction (it never resets), so
  the arithmetic `head = write_count % capacity` automatically wraps around.
- **...but no wrap.** The buffer starts empty and considers a slot "populated"
  only after its first write (`populated` vector). `is_full()` is true once
  `write_count >= capacity`, i.e. after it has wrapped at least once.
- **Time index.** Every `push(time)` records the write time. Lookups scan from
  the newest slot backwards toward the oldest (at most `capacity` slots), which
  matches most simulation access patterns ("most recent first").
- **Internals.** A single contiguous block is held by
  `AlignedBufferPool` (`buffers`), which exposes one `locations[i]` pointer per
  slot; the timestamps/populated flags live in parallel vectors.

> Note: there are deliberately almost no bounds checks in this class — it is a
> hot-path data structure. Callers must pass valid indices. The couple of
> checks that exist (`get_item`, `get_time`) throw on unpopulated slots.

## State fields

| Field | Type | Purpose |
|---|---|---|
| `log` | `Logger*` | Categorized logger for this instance. |
| `timestamps` | `vector<uint64_t>` | Per-slot write time. |
| `populated` | `vector<bool>` | Whether each slot has been written at least once. |
| `buffers` | `AlignedBufferPool` | Owns the contiguous byte storage. |
| `head` | `size_t` | Index of the most recent write. |
| `capacity` | `size_t` | Total usable slot count. |
| `write_count` | `size_t` | Cumulative number of `push` calls (never decremented). |

---

## Function-by-function reference

### `RingBuffer(size_t capacity, size_t buffer_size)`

Constructor. Initializes the timestamp/populated vectors to `capacity` empty
slots, allocates the aligned byte pool, and marks every slot unpopulated.

- Throws `std::runtime_error` if `capacity == 0`.
- The backing store is `AlignedBufferPool(buffer_size, capacity)`.

### `~RingBuffer()`

Destructor. Logs a TRACE on teardown. Owned buffers are released by their
owning types (vectors / pool).

### `size_t push()`

Writes the **next** slot (no timestamp assignment) and returns its index.

- Increments `write_count`, sets `head = write_count % capacity`, marks that
  slot as `populated`, and returns `head`.

### `size_t push(uint64_t time)`

Writes the next slot **with a timestamp**, then returns the new head index.

- Writes `time` into `timestamps[(write_count + 1) % capacity]` (the slot that
  is about to become `head`) **before** advancing `head`. Ordering matters: if
  the timestamp were written after advancing `head`, a concurrent
  `find_latest_valid_index` could observe a stale old time at the new head and
  return a wrong result.
- Implementation delegates to the no-arg `push()` after the timestamp write.

### `byte *get_item(size_t index, bool use_verification = true)`

Returns the pointer to slot `index` so the caller can read/write raw bytes.

- If `use_verification` and the slot is not `populated`, logs an error and
  throws `std::runtime_error("[RingBuffer][get_item] Index not populated")`.
- Returns `buffers.locations[index]` otherwise.

### `uint64_t get_time(size_t index)`

Returns the recorded timestamp for slot `index`.

- Throws `std::runtime_error("[RingBuffer][get_time] Index not populated")` if
  the slot was never written.
- Note: unpopulated slots may still hold stale zeros/older times, so this guard
  matters.

### `bool find_exact_index(uint64_t time, size_t &index_found)`

Exact timestamp match. Returns `true` and writes the matching slot to
`index_found`, or `false`.

- Captures `write_count` into a local first so a concurrent bump cannot cause a
  slot to be missed or processed twice.
- Scans `i = 0 .. min(write_count, capacity)-1`, testing
  `timestamps[(write_count - i) % capacity] == time` (newest first).

### `bool find_latest_valid_index(uint64_t time, size_t &index_found)`

Returns the newest slot whose timestamp `<= time`.

- Same newest-first walk as `find_exact_index` but matches on
  `timestamps[pos] <= time`.
- Useful for "give me the most recent data point at or before time `t`".
- Returns `false` if no such slot exists.

### `bool find_next_valid_index(uint64_t time, size_t &index_found)`

Returns the newest slot whose timestamp is **strictly greater than** `time`
(`timestamps[pos] > time`).

- The inverse of `find_latest_valid_index`; useful for "first data point after
  time `t`". Returns `false` if none exists.

### `size_t index_back_from_head(size_t position)`

Maps a logical backwards position to a physical slot index: `position 0` equals
`head` (newest), `1` is just before head, `2` is two back, etc.

- Computed as `(write_count - position) % capacity`.

### `bool is_empty()`

Returns `true` when nothing has been written yet (`write_count == 0`).

### `bool is_full()`

Returns `true` once the buffer has wrapped at least once
(`write_count >= capacity`).

### `string to_string() const override`

Returns a short human-readable summary: capacity, `write_count`, and `head`.
(Note: the header/body text currently reuses the literal `"SignalStorage"` as a
label; this is a cosmetic quirk of the current implementation.)