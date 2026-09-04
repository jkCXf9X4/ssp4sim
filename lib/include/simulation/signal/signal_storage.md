# `ssp4sim::signal::SignalStorage`

A **data-centric, time-indexed storage area** for simulation signals. It holds,
for one single-producer data stream, a fixed number of time-versioned *areas*
(e.g. one area per simulation step). Every area stores the same set of typed
variables plus optional interpolation derivative slots, laid out in contiguous
memory for fast access and easy export.

- Namespace: `ssp4sim::signal`
- Header: `lib/include/simulation/signal/storage.hpp`
- Backing store: one `ssp4sim::utils::RingBuffer` per `SignalStorage`
  (see [`ring_buffer.md`](../../utils/primitives/ring_buffer.md)).
- Implements: `types::IWritable` (i.e. supplies `to_string()`).

## Purpose

The storage area enables:

1. **Easy access when exporting results** — each variable's value and its
   derivatives sit at a known, cached byte offset per area.
2. **Altering data in/out of the model** — readers/writers locate a value by
   `(area, variable index)` and poke the raw bytes directly.
3. **Backwards-in-time access** — multiple time versions are kept in the ring,
   so callers can query the value at an earlier timestamp (exact match, latest
   at-or-before, or next-after).

`SignalStorage` is used per model I/O area: e.g. an FMU model owns an `input`
area and an `output` area (see `pre/3_simulation/elements/model_fmu.cpp`), and
a `DataRecorder` can subscribe to new-data events to persist signals.

## Data model

### `SignalInfo`

Describes one logical variable within an area:

| Field | Meaning |
|---|---|
| `index` | Position of the variable in the `variables` vector. |
| `name` | Human-readable variable name. |
| `type` | `types::DataType` (real / integer / boolean / enumeration / string). |
| `type_size` | Byte size of one value of that type. |
| `type_alignment` | Required memory alignment of the value. |
| `max_interpolation_orders` | Highest derivative order available (0 = none). |
| `derivative_size` | Byte size of one derivative (always `sizeof(double)`). |
| `derivative_alignment` | Alignment of derivative slots (double alignment). |
| `total_size` | Bytes occupied by this variable in a single area (`position..end`). |
| `position` | Byte offset of the value within the area's data chunk. |
| `derivative_position` | Byte offset of the **first** derivative within the chunk. |

### Layout

For each area, variables are laid out back-to-back in one contiguous byte
chunk. Per variable:

```
value at      position            (type_size bytes)
derivative[1] at derivative_position + 0 * derivative_size
derivative[2] at derivative_position + 1 * derivative_size
 ...
derivative[n] at derivative_position + (n-1) * derivative_size
```

Each field is `align_up`'d to its required alignment, and the running
`area_byte_size` (one area's total byte size) is aligned up to
`alignof(max_align_t)`.

### Memory / references cached after `allocate()`

- `ring` — the owning `RingBuffer` (one slot per area).
- `locations[area][var]` — absolute `byte*` of the value for that area/variable.
- `derivative_locations[area][var]` — absolute `byte*` of the first derivative.
- `area_byte_size` — bytes consumed by each single area.

Because these pointers are precomputed once at `allocate()` time, runtime access
is a single array lookup with no per-call offset math.

## New-data notifications

A single callback can be registered per storage. When an area is written and
flagged, a `NewDataEvent` is produced that carries the storage, the area index,
its timestamp, and (filled in later by the recorder) the destination buffer and
recorder buffer index. This is the mechanism that connects `SignalStorage`
productions to `DataRecorder` sinks.

## State fields

| Field | Type | Purpose |
|---|---|---|
| `log` | `Logger*` | Categorized logger. |
| `areas` | `size_t` | Number of time-versioned areas (ring capacity). |
| `name` | `string` | Storage name (used for logging/debugging). |
| `allocated` | `bool` | Whether `allocate()` has run. |
| `ring` | `unique_ptr<RingBuffer>` | Owns the actual time-stamped areas. |
| `variables` | `vector<SignalInfo>` | Metadata for every stored variable. |
| `area_byte_size` | `size_t` | Byte size of one area's data chunk. |
| `locations` | `vector<vector<byte*>>` | `[area][var]` value pointers. |
| `derivative_locations` | `vector<vector<byte*>>` | `[area][var]` first-derivative pointers. |
| `new_data_callback` / `new_data_callback_context` | — | Registered notification hook. |

---

## Function-by-function reference

### `SignalStorage(size_t areas, string name)`

Constructor. Records the ring capacity (`areas`) and a human-readable `name`,
and creates the logger. Memory is **not** allocated yet — call `add_variable()`
for each variable and then `allocate()`.

### `~SignalStorage()`

Destructor. Only acts if `allocate()` ran. For every area and every `string`
variable it calls `std::destroy_at` on the stored location, because strings are
placed in raw (non-GC'd) memory and must be released explicitly. Non-string
types need no per-slot teardown.

### `size_t add_variable(string name, types::DataType type, size_t max_interpolation_order)`

Declares a new variable of the given `type` with room for
`max_interpolation_order` derivative slots. Computes and records the full layout
(positions, sizes, alignments), updates the running `area_byte_size`, and
appends a `SignalInfo`.

- Returns the new variable's **index** (its position in `variables`).
- Must be called before `allocate()` for every variable you intend to store.

### `void allocate()`

Allocates the backing ring and precomputes every value/derivative pointer.

- Creates `ring = RingBuffer(areas, area_byte_size)`.
- Resizes `locations` / `derivative_locations` to `[areas][#variables]`.
- For each area: obtains the area's slice from the ring and fills the per-variable
  value and first-derivative pointers from the precomputed offsets.
- For `string` variables, runs `std::construct_at` to initialize the string in
  place (companion to the destructor teardown).
- Sets `allocated = true`.
- Throws `std::runtime_error("Buffer can only be allocated once")` if called twice.

### `size_t push(uint64_t time)`

Advances to a new area stamped with `time` and returns that area's index.
Thin wrapper over `ring->push(time)`.

### `size_t get_or_push(uint64_t time)`

Returns the area already matching `time` if one exists, otherwise `push`es a new
area for that time and returns its index. This avoids duplicate areas for a
timestamp that was already written.

### `bool find_area(uint64_t time, size_t &found_index)`

Exact time lookup. Delegates to `ring->find_exact_index(time, found_index)`.
Returns `true` (and writes the matching area) if an area for that exact time
exists.

### `bool find_latest_valid_area(uint64_t time, size_t &found_index)`

Finds the newest area with timestamp `<= time`. Delegates to
`ring->find_latest_valid_index`. Returns `false` if none exists (e.g. buffer
empty or all areas newer than `time`).

### `bool find_next_valid_area(uint64_t time, size_t &found_index)`

Finds the newest area with timestamp **strictly greater than** `time`. Delegates
to `ring->find_next_valid_index`. Returns `false` if none exists.

### `uint64_t get_time(size_t area)`

Returns the recorded timestamp of an area. Delegates to `ring->get_time(area)`.
May throw if the slot was never populated.

### `byte *get_item(size_t area, size_t index) noexcept`

Returns the cached **value** pointer `locations[area][index]`, or `nullptr` if
the storage was never allocated. Hot-path; no other validation. Use the returned
pointer to read/write the variable's raw value.

### `byte *get_derivative(size_t area, size_t index, size_t order) noexcept`

Returns a pointer to the `order`-th derivative of variable `index` in `area`, or
`nullptr` on any invalid input.

- `order == 0` returns `nullptr` (order 0 is the value itself, not a derivative).
- `order > max_interpolation_orders` (or `max == 0`) returns `nullptr`.
- `index` out of range, unallocated storage, or a null derivative location all
  return `nullptr`.
- Guards are compiled **only** under `SSP4SIM_HOT_PATH_CHECKS`; otherwise the
  function computes `derivative_locations[area][index] + (order - 1) * sizeof(double)`
  directly.
- **Note:** derivative offsets depend on the value type's layout and
  interpolation ordering; validate `max_interpolation_orders` when using this.

### `void register_callback(Callback cb, void *context)`

Registers a single `NewDataEvent` callback (`cb`) with its `context`. Only one
callback is stored; a later call replaces the previous one.

### `void flag_new_data(size_t area)`

Announces that `area` has fresh data. If the storage is allocated **and** a
callback is registered, builds a `NewDataEvent` (storage, area, current
timestamp) and invokes the callback with its context. The recorder hooks this to
pick up newly written data for export.

### `string to_string() const override`

Returns a human-readable summary: name, area count, allocation state,
`area_byte_size`, variable count, and one entry per variable (position, name,
type, size).

### `string export_area(int area)`

Serializes a single area for debugging/export: for each variable it dumps its
position, derivative position, order count, name, type, size, and the current
**value** (formatted via the FMI2 type-to-string helper). Unallocated areas or
non-values are not specially guarded here.