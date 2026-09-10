# Test Levels Analysis — Coverage Map and Future Implementation

// Flag this as a todo if you read this

Status: analysis only, no code changes. Written as the follow-up to the
per-test-build refactor (`test-fragment-plan.md`); captures where each test
level is exercised today and the concrete implementation work needed to close
the identified gap.

Related: `tests/README.md`, `tests/lib/CMakeLists.txt`,
`tests/lib/cmake/AddUnitTargets.cmake`, `test-fragment-plan.md`.

## 1. Level taxonomy in this repository

| Level | Runs where | What exercises it today |
|---|---|---|
| **Unit** | 15 standalone `test_*` binaries (cone-isolated, built from their own include closure) | primitives, parser elements, schema extensions, analysis builders (`tree_builder`, `graph_builder`, `ssp_node`) |
| **Component** | the 8-file monolith `ssp4sim_tests` (linked against full `ssp4sim_lib`) | C++ tests that need >1 lib module at once: recorders/sinks, resolver, fmi4c adapter, sim-graph construction |
| **Integration** | Python `tests/python/high_level/*` (4 files, ~771 lines) | full pipeline via the built `py_ssp4sim` extension, real SSP/FMU fixtures, result comparison |
| **E2E** | `test_smoke_simulator.cpp` (1 TEST_CASE) + `test_pyssp4sim_cli.py` | the only top-to-bottom run through the public entry point |

Monolith test-case breakdown (49 total):

| File | TEST_CASE blocks |
|---|---|
| `scheduling/test_read_target_resolver.cpp` | 13 |
| `graph/test_sim_graph_builder.cpp` | 10 |
| `core/test_data_recorder.cpp` | 7 |
| `core/test_fmi4c_adapter.cpp` | 7 |
| `core/test_sqlite_recorder.cpp` | 7 |
| `simulation/test_sim_graph_builder.cpp` | 3 |
| `high_level/test_smoke_simulator.cpp` | 1 |
| `model/test_model_connection.cpp` | 1 |

## 2. Verified coverage inventory (as of this analysis)

- Unit suite: 15 binaries, ~80 test cases (`tests/run_unit.sh --tier T1|T2|T3`).
- Component: monolith, 49 test cases, registered in CTest.
- Integration/E2E: Python `high_level/` (4 files) + smoke + CLI test.
- CTest total: 129 cases, all green.

### The critical finding (verified, stronger than expected)

**No C++ test file under `tests/lib/` references the executor layer at all.**
`grep -rn 'executor|jacobi|seidel' tests/lib --include='*.cpp'` returns
nothing. The executor family — `simulation/graph_executor/execution/**`
(`jacobi_*`, `seidel_*`, `loop_aware_executor`) — is exercised only by:

1. the Python integration tests (`test_spike_regression.py`, `test_loop_aware_nested.py`), and
2. transitively, the smoke/E2E path.

Consequences:

- A correctness regression in `graph_executor/**` is invisible to every C++
  test. The per-test cone isolation means unit tests cannot even *accidentally*
  cover it; the monolith links it but asserts nothing about scheduler behavior.
- The Python suite is the sole oracle for the exact subsystem this repo's
  refactoring effort treats as most fragile — and it carries the whole-suite
  coupling failure mode (broken executor file → `py_ssp4sim` fails to build →
  `pytest -q tests/python` collapses).
- This inverts the pyramid at the component level for exactly the layer with
  the most risk.

## 3. Why the cone machinery is the enabling piece

The per-test cone build is precisely what makes closing this gap cheap and
safe:

- A standalone executor test would pull only its own cone
  (`simulation/graph_executor/execution/**` + shared utils), so a break in
  `jacobi_parallel_tbb.cpp` cannot block `test_jacobi_serial`. That is honest
  isolation, the same property verified for the kernel tiers.
- The cone module, `ssp4sim_cone_audit()` and CTest registration already exist;
  adding new binaries is a **data** change (`tests/lib/CMakeLists.txt` + a new
  `test_*.cpp`), not machinery work.

## 4. Implementation plan (future work)

### 4.1 Executor component benches (priority 1)

New directory `tests/lib/executor/` with cone-built unit binaries that
validate scheduler *behavior* without FMUs, the monolith, or Python:

- `tests/lib/executor/test_jacobi_serial.cpp` — feed a small DAG with a known
  fixed-point/steady-state; assert convergence and schedule order.
  Includes: `simulation/graph_executor/execution/jacobi/jacobi_serial.hpp` (cone
  = jacobi serial + executor base + invocable + utils/graph).
- `tests/lib/executor/test_seidel_serial.cpp` — same fixture, seidel order
  semantics; assert in-place updates differ from jacobi (both read results of
  the same graph).
- `tests/lib/executor/test_loop_aware_scheduling.cpp` — algebraic-loop
  regression in C++ at the unit level: nested loop SCC, fixed/geometric
  sub-step modes, steady-state compare (mirror of
  `test_loop_aware_nested.py` but without fixture archive I/O).

All three become standalone `test_jacobi_serial`, `test_seidel_serial`,
`test_loop_aware_scheduling` binaries automatically (GLOB + `add_unit_test`);
register in `tests/run_unit.sh` `--tier T` bucket and the
`tests/README.md` tier matrix.

Acceptance:

- `ninja -C build test_jacobi_serial && ./build/tests/lib/test_jacobi_serial`
  builds & passes **while `jacobi_parallel_tbb.cpp` is intentionally broken**.
- Full `ctest --test-dir build` still 129 + new cases green.

### 4.2 Resolver/scheduling promotion (priority 2)

`test_read_target_resolver.cpp` (13 cases) is currently monolith-bound, but its
cone is clean (`scheduling/read_target_resolver.hpp` + `read_target_core.hpp` +
introduced types). Promote it to a standalone binary so the 13 resolver cases
are build-time isolated from the sim layer too.

### 4.3 Level-boundary hygiene (priority 3, optional)

- Keep `test_smoke_simulator.cpp` as the single true E2E (per
  `tests/README.md` guidance).
- Keep Python as integration/result-comparison oracle; do **not** duplicate its
  fixture-grounded sweeps in C++.
- If the executor benches (4.1) land, re-triage which of the remaining 8
  monolith files genuinely need `ssp4sim_lib` vs. can be promoted to cones —
  shrinking the monolith's blast radius further.

## 5. Explicit non-goals / decisions preserved

- No FMU loading or fixture-archive I/O in component benches (that stays in
  Python integration).
- No new machinery: benches reuse `AddUnitTargets.cmake` unchanged.
- No change to the integration binary's honest coupling — monolith tests that
  need the whole pipeline stay linked to `ssp4sim_lib` on purpose.
- `test_sim_graph_builder.cpp` (graph/ and simulation/ variants) both stay in
  the monolith; the executor benches are the new, behavioral component layer.

## 6. Verification / acceptance (summary)

1. Executor benches build, link and pass as standalone binaries.
2. Isolation re-verified with one injected break in an executor sibling file —
   serial tests stay green, the broken file's honest dependents fail.
3. `ctest --test-dir build --output-on-failure -j` passes (new case count =
   previous 129 + new executor/resolver cases).
4. Python integration suite still green (no regression from added C++ tests).
5. `tests/README.md` tier matrix and `tests/run_unit.sh` buckets updated.