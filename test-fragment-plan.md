# Plan: Fragment the C++ test suite into per-test binaries

Status: implemented (Phases 0–2). See `## Implementation status & deviations`.
Scope: `tests/lib/` build architecture + execution surface. Python suite unchanged.
Related docs: `tests/README.md`, `docs/build_from_source.md`, `docs/development.md`, `tests/lib/CMakeLists.txt`, `lib/CMakeLists.txt`

## 1. Context & problem

The repository's dominant failure mode today: **one broken implementation file makes the entire test suite unbuildable.**

- `lib/CMakeLists.txt:5` GLOBs all 66 `.cpp` files into one static `ssp4sim_lib`.
- `tests/lib/CMakeLists.txt:1-3` GLOBs all 33 test files into one 121 MB `ssp4sim_tests` executable.
- Python tests (`tests/python/conftest.py:18-21`) require the built full-pipeline extension.

During the in-progress executor refactor (39 modified files across `la2`/`seidel`/`jacobi`/`executor`), a break in any one `simulation/execution/**` file blocks `tests/lib/utils` and `tests/lib/graph` from even compiling. "Targeted testing" currently means run-time filters only (Catch2 `--filter`, `ctest -R`); it is never build-time targeted.

## 2. Goals

1. Build-time isolation: a unit test compiles and runs from its own source cone, without the complete codebase.
2. Results understandability: one unit binary = one failure = one clear per-test result, through a canonical CTest surface (also fixes the documented "`ctest` is currently unreliable" at `tests/README.md:42`).
3. Permanent architecture: structure survives future refactors; no ad-hoc scaffolding.
4. Honest isolation: tests that legitimately need the broken modules still fail, and the report shows exactly which.

## 3. Non-goals

- No changes to `tests/python/**` or the Python extension linkage. High-level workflow tests are inherently full-pipeline and stay coupled.
- No logging seam: kernel tests may link `ssp4cpp`/quill for logging (accepted decision; it is the current runtime reality).
- No source moves of `lib/**`; the fix is entirely in build logic.

## 4. Rationale

### 4.1 Why per-test executables rather than per-layer libraries

Per-layer static libs (splitting `ssp4sim_lib` into `utils/parser/analysis/simgraph/sim`) would be **insufficient alone**: a broken file inside a layer still blocks every test in that layer, and a monolithic GLOB keeps one fragile link. Per-test cones push isolation to the actual unit boundary, matching the repository's *tested unit* granularity. This is what makes the current WIP refactor survivable: kernel tests (`graph_analysis`, `ring_buffer`, resolvers, node) compile and pass while `simulation/execution/**` is red.

Per-test cones therefore make the **layer split a non-prerequisite**: the isolation it was proposed to provide is already delivered by cone pruning. The layered libs retain only a narrow, opportunistic role — a T3 sim/graph test whose cone would massively duplicate shared code may link one layer lib instead. That is a Phase 2.5 optimization, not a Phase 1 gate. What must stay intact is the **aggregate `ssp4sim_lib`** (or any single static target) so `public/python_api` and `public/ssp4sim_app` link unchanged — a hard non-goal.

### 4.2 Why the sibling-mapping cone rule is safe here

The whole automation hinges on one verified invariant: implementation `.cpp` files sit adjacent to their `.hpp` under `lib/include/**` **in normalized layout** (59/71 headers have a same-name sibling `.cpp`; 12 are header-only and contribute nothing). Interior normalization (e.g. `3_simulation` → `3_simulation_graph`, `2_analysis` → `2_analysis_graph`) exists and must be respected by shared_config — see risks below. With that invariant, include→file resolution is mechanical: parse `#include "..."`, strip comments, strip `<...>`, canonicalize, find the `.hpp`, map to the `.cpp` sibling, close transitively.

### 4.3 Why fixed link profiles, not per-cone dependency sniffing

Per-include `find_package` dependent linking multiplies CMake complexity linearly with cone variance and is the main source of cost. Instead: two static link profiles. Kernel: Catch2 + `ssp4cpp` + quill + nlohmann_json. Integration (retained single binary): kernel profile + TBB + sqlite + fmi4c. The cost — every kernel binary links ssp4cpp/quill even when the cone does not need them — was accepted knowingly. Keeps the module linear.

### 4.4 Why CTest as the canonical aggregator

30 isolated binaries would otherwise leak 30 result streams. CTest already exists in the tree (`tests/lib/CMakeLists.txt:45` `catch_discover_tests`, `build/tests/CTestTestfile.cmake`); per-test binaries make it reliable per entry (a crashing/failing test no longer aborts a shared monolithic process). `ctest --test-dir build/tests --output-on-failure -j` yields the single tabular PASS/FAIL view; `ctest -R` gives targeted runs. This directly resolves the known unreliability and preserves whole-suite readability.

## 5. Design

### 5.1 Retained library axes

Keep the existing aggregate static `ssp4sim_lib` (or an equivalent single static target) so `public/python_api` and `public/ssp4sim_app` link unchanged. No source moves, no include-path changes. **No mandatory layer split.** If a T3 sim/graph cone proves bloated during Phase 1, split a single layer out of `ssp4sim_lib` as an optional link target for that test — an opportunistic Phase 2.5 step, never a prerequisite.

### 5.2 The cone module (Phase 1)

New file `tests/lib/cmake/AddUnitTargets.cmake` with function `add_unit_test(<cpp>)`:

1. Comment-strip each impl header; regex `#include "..."`; drop `<...>`.
2. Canonicalize the include path: try as-given against `lib/include/**`, then normalized spellings from `shared_config.hpp`; resolution is **soft** — unresolvable → treat as header-only, skip.
3. Map `.hpp` → sibling `.cpp`, dedupe by canonical realpath, close transitively with a visited set, cap depth.
4. `add_executable(<basename> kernel_main.cpp <cpp> <cone...>)`; `basename` = test file stem; collision policy: same cone-target name from different dirs → target name collision, resolve by explicit list.
5. Link profile by tier membership:
   - Kernel/parser/graph (pure units): Catch2 + `ssp4cpp` + quill + nlohmann_json.
   - Integration (retained binary): + TBB + sqlite + fmi4c.
6. `catch_discover_tests(<basename>)` per target; keep `SSP4SIM_PROJECT_ROOT` and the existing include-dir set.

`kernel_main.cpp`: ~6-line Catch2 v3 `Session` runner (mirror of `tests/lib/test_main.cpp:6-11` minus forced quill console; logging available but not a build requirement).

`tests/lib/CMakeLists.txt` shrinks to: `foreach` over kernel test files → `add_unit_test`, plus retained `ssp4sim_tests` built from an explicit integration list (kernel files excluded). `GLOB CONFIGURE_DEPENDS` retained for discovery so new files are picked up; no hand-maintained cone lists.

### 5.3 Execution surface (Phase 2)

- `tests/run_unit.sh --tier T1|T2|T3` and `--filter <name>`: thin sugar over CTest + per-binary run.
- CTest is canonical: register all unit targets; document `ctest --test-dir build/tests --output-on-failure -j`.
- Optional: uniform `--rng-seed` pass-through for reproducible seeds; JUnit/XML merge step deferred until a CI consumer exists.

## 6. Phasing

- **Phase 0 — classify (no code moves):** scan include cones of all 33 test files; bucket into tiers; produce the tier matrix in `tests/README.md`; identify near-duplicates (e.g. `test_read_target_resolver_patched.cpp` vs plain) and whether to keep/merge. Product: the cone inventory + the concrete acceptance break-file.
- **Phase 1 — cone module + per-test targets:** implement `AddUnitTargets.cmake`, `kernel_main.cpp`, convert kernel/parser/graph tiers; migrate integration list. Aggregate `ssp4sim_lib` stays intact for python/app.
- **Phase 2 — execution + docs:** `tests/run_unit.sh`, CTest registration, update `tests/README.md`, `docs/build_from_source.md`, `docs/development.md`, root `AGENTS.md` quick commands.
- **Phase 2.5 — optional:** if a T3 sim/graph cone proves bloated, split only the needed layer out of `ssp4sim_lib` as a link target for that test. Only done on measured need; never a prerequisite.

## 7. Verification / acceptance

With the currently-broken WIP files **kept broken**:
1. `ninja test_graph_analysis && ./build/tests/lib/test_graph_analysis` → passes.
2. `ninja test_ring_buffer && ./build/tests/lib/test_ring_buffer` → passes.
3. A T3/T4 test depending on the broken files → still fails (isolation is honest).
4. `ctest --test-dir build/tests --output-on-failure -j` → per-unit PASS/FAIL table; running set exactly matches expected kernels; crashes isolated.
5. `cmake --build build` still produces app + python extension unchanged in behavior.
6. Full-suite nuclear option (`cmake --build build` + `ctest` + `pytest -q tests/python`) still works end-to-end.

## 8. Risks & mitigations

- **Non-canonical include spellings** (verified: `3_simulation/...` referenced 7× though the dir is `3_simulation_graph`; `2_analysis/...` 5× though it is `2_analysis_graph`). Mitigation: `shared_config.hpp` spelling map + **soft resolution**; outcome of a wrong guess is a per-binary link-time undefined-reference, which is clear, not silent.
- **Commented-out includes** (verified: `fmu_info.hpp:7`). Mitigation: comment stripping before regex; otherwise duplicate compilation of siblings.
- **Stale cones at configure time** (`CONFIGURE_DEPENDS`). Mitigation: never hand-edit cone lists; reconfigure is part of the build loop; document the rule.
- **Target-name collisions** (two `test_*.cpp` with same stem in different dirs). Mitigation: explicit allow-list override or directory-prefixed names.
- **Per-binary duplicate compile cost.** Cone-sharing is small (resolver headers shared by ≤3 tests); clean build +10–20% est.; incremental builds dominate anyway. Accepted.
- **CMake floor:** `GLOB_EXCLUDE` needs ≥3.12; if root 3.10 must stay, use explicit integration list instead.

## 9. Files touched

- New: `tests/lib/cmake/AddUnitTargets.cmake`, `tests/lib/kernel_main.cpp`, `tests/run_unit.sh`, `test-fragment-plan.md` (this).
- Modified: `CMakeLists.txt` (root `enable_testing()`), `tests/lib/CMakeLists.txt`, `tests/README.md`, `docs/build_from_source.md`, `docs/development.md`, `AGENTS.md`.
- Optional (Phase 2.5, only on measured need): `lib/CMakeLists.txt` if a T3 sim/graph test links a split-out layer.
- Not needed: `lib/include/shared_config.hpp` spelling map — the current tree already uses normalized directory names (`2_analysis`, `3_simulation`), so no spelling normalization exists in practice.

## 11. Implementation status & deviations

Implemented Phases 0–2: 15 standalone per-test binaries + retained integration
binary; `ctest --test-dir build` passes 129/129; `tests/run_unit.sh --tier T1|T2|T3`
and `--filter` work. Deviations from the draft:

- **`kernel_main.cpp` keeps the console sink.** The draft said "minus forced quill
  console"; in practice cone sources construct quill loggers unconditionally, so
  without a sink every logged test fails with "Tried to create logger without
  sinks". Follows the accepted "no logging seam" decision.
- **Local angle-bracket includes are resolved.** `test_node_iterator.cpp` includes
  `node.hpp` as `<utils/primitives/node.hpp>`; the cone parser retains `<...>`
  targets and resolves them against the include set, dropping only unresolvable
  system/external headers.
- **`enable_testing()` moved to the root `CMakeLists.txt`** so the canonical
  `ctest --test-dir build` returns the whole suite (the plan's literal
  `--test-dir build/tests` yielded zero tests because CTest was only enabled in
  the `tests/lib` subtree).
- **Counts differ from the plan text.** The tree has 23 test files (not 33), no
  `test_graph_analysis.cpp` (the standalone analysis entry points are
  `test_graph_builder`, `test_tree_builder`, `test_ssp_node`), and no
  `_patched.cpp` resolver duplicate. Unit scope = 15 binaries, integration = 8.
- **Isolation verified with an injected break** in
  `lib/include/simulation/graph_executor/execution/jacobi/jacobi_serial.cpp`:
  standalone binaries still built; `ssp4sim_tests` failed (honest coupling).
  Reverted.
- `tests/run_unit.sh` reclassifies the tier lists manually (GLOB picks up new
  test files automatically, but they only join a tier bucket when added there).

## 12. Review follow-ups (P0–P2, implemented)

Critical-review findings and their resolutions:

- **P0 — block-comment false positives fixed.** `ssp4sim_parse_quoted_includes`
  now strips `/* ... */` comments (newline-preserving) before the include scan;
  previously a `#include` at line start inside a block comment entered the cone.
  Verified against a comment fuzz specimen; the configure-time audit reports all
  15 cones clean.
- **P1 — LTO-off attempted and reverted (documented tradeoff).** Disabling LTO
  on unit binaries is infeasible: `ssp4cpp` is itself built with `-flto=auto`
  (its own `CMAKE_CXX_FLAGS_RELEASE`), so the unit link must invoke the LTO
  plugin; a `-fno-lto` link fails with "plugin needed to handle lto object".
  Clean-build cost is therefore accepted and measured: full tree
  (`ssp4sim_lib` + unit binaries + integration + python + app) ≈ **2:05 wall /
  600 s CPU @ -j4**. The cost driver is repeated `-O3 -flto` compilation and
  link of shared cone sources (leaves compiled up to 9×).
- **P1 — redundant `find_package(Catch2)` removed** from the integration
  section; package discovery happens once at the `tests/lib` scope.
- **P1 — reconfigure rule documented.** `#include` edits inside an existing
  cone source do not retrigger `CONFIGURE_DEPENDS`; noted in
  `tests/lib/CMakeLists.txt` and `tests/README.md`.
- **P2 — configure-time cone audit.** Every configure emits
  `[cone audit] OK: N unit cones, M lib sources ...` (STATUS) or per-test
  `WARNING` for local includes a cone cannot resolve (a directory missing from
  `SSP4SIM_TEST_INCLUDE_DIRS`, or a misspelled include). This makes the cone
  machinery self-checkable and gives CI a single visible marker.
- **P2 — single source of truth for the run set.** `tests/run_unit.sh` default
  derives from the build tree (any built `test_*` binary is run), so new tests
  are executed without script edits. Tier buckets T1–T3 remain the curated
  Phase 0 grouping; a built binary outside all tiers triggers a drift warning
  instead of being silently skipped. Seed data stays in `tests/README.md` and
  `run_unit.sh` in parallel.

## 13. ccache (on by default)

Follow-up to §12's compile-reuse discussion: `ccache` is wired as the compiler
launcher and is **on by default** — every local/container/CI configure that
finds `ccache` on PATH enables it without a flag.

- Wiring: `cmake/ccache.cmake`, loaded via `CMAKE_PROJECT_INCLUDE` from the
  `vcpkg` preset. `-DCCACHE=OFF` disables; the tool just not being installed
  builds without it (STATUS message, never an error).
- Cache location: `~/.cache/ccache` (default) or `CCACHE_DIR`. Host and
  container caches are deliberately **not shared** — inside a container the
  cache lives in the throwaway home (`/tmp/ssp4sim-home/.cache/ccache`), so
  each container starts cold; ccache still helps repeated builds within one
  container session. CI therefore has no ccache persistence step. The
  `vcpkg-ccache` preset was dropped as redundant once default-on landed.
- Verified: warm T1+T2 rebuild 12.2 s → 4.7 s (~2.6×), 41% compile hits;
  129/129 ctest on a ccache-built tree. First clean build is unchanged (ccache
  computes before it caches) and per-binary LTO links still dominate runtime —
  the residual cost from §12.

## 10. Decisions

- Isolation granularity: **per-test executables** built from each test's own source cone.
- Scope: permanent test-architecture change; verified against the current WIP break.
- ssp4cpp/quill in kernel tests: **accepted** as link-time cost; no logging seam.
- Cone mechanism: **automated transitive include closure**; T3 sim/graph may optionally link a split-out layer lib only where cone-sharing would duplicate heavily (Phase 2.5, on measured need).
- Result aggregation: **CTest canonical**, `tests/run_unit.sh` as thin sugar.
- Near-duplicate resolver tests (`test_read_target_resolver.cpp` / `_patched.cpp`): **keep both; flag only** during Phase 0, no pre-emptive merge.
- Save path: `docs/test-fragment-plan.md`.