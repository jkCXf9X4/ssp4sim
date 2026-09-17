# Bug Report: pyssp4sim 0.3.0 deletes the extracted FMU before loading it

**Component:** ssp4sim (upstream) / `pyssp4sim` Python wheel
**Affected artifact:** `pyssp4sim-0.3.0-cp39-abi3-linux_x86_64.whl` (GitHub release download)
**Wheel SHA-256:** `d8558d73ac262b12780f438a533d1f15334368c482fac1d3619d13b5b603d739`
**Report date:** 2026-09-17
**Reported from:** `sim_arch_comparison` benchmark repository

## Summary

The v0.3.0 release wheel cannot instantiate **any** FMU referenced from an SSP
`resources/*.fmu` archive. After the engine unzips the FMU into its temporary
directory (`/tmp/ssp4cpp_fmi_*`), it deletes every extracted file again before
`chmod +x` and the shared-object load, so the load always fails with
`Failed to load functions for FMI 2`. This regresses the v0.2.11 wheel, which
resolved the earlier FMU-extraction blocker (see `.dynamic-harness/findings.md`,
claim 38 / A10 and `docs/roadmap/study2-maturity.md` §6.5).

## Reproduction

With the v0.3.0 wheel installed in the repo-local `.venv`:

```bash
python3 -m experiments.src.run_experiment simulate --study simple_benchmark --force
# or any single config:
build/.venv/bin/python -c \
  "import sys, pyssp4sim; sim = pyssp4sim.Simulator(sys.argv[1]); sim.init(); sim.simulate()" \
  build/simple_benchmark/configs/monolithic_reference.json
```

Observed error (identical for simple_benchmark and synthetic_propagation, all
configs, all executor methods):

```
chmod: cannot access '/tmp/ssp4cpp_fmi_XXX/binaries/linux64/MonolithicReference.so': No such file or directory
Loading shared object failed: /tmp/ssp4cpp_fmi_XXX/binaries/linux64/MonolithicReference.so
  (cannot open shared object file: No such file or directory)
RuntimeError: Failed to instantiate FMU '/tmp/ssp4cpp_fmi_XXX': Failed to load functions for FMI 2.
```

## Root cause (from `strace`)

The engine lifecycle for an FMU archive is:

1. `mkdir /tmp/ssp4cpp_fmi_<rand>` and write every zip entry there
   (`openat(..., O_WRONLY|O_CREAT|O_TRUNC)`, then `chmod 0644`).
2. **Delete every written file again** (`unlinkat` on the same directory fd,
   including `binaries/linux64/*.so`, `modelDescription.xml`, `sources/*`, …).
3. Run `chmod +x <binary>` on the now-missing `.so` → `cannot access`.
4. `dlopen` the now-missing `.so` → `cannot open shared object file`.

Confirmed by tracing a single-config run:

```
104  openat(.../binaries/linux64/PassThroughNode_n10.so, O_WRONLY|O_CREAT|O_TRUNC) = 7   # extract
1029 unlinkat(8, "PassThroughNode_n10.so", 0) = 0                                        # delete
1046 execve("/usr/bin/chmod", ["chmod", "+x", .../PassThroughNode_n10.so])               # chmod (fails)
1079 openat(.../binaries/linux64/PassThroughNode_n10.so, O_RDONLY|O_CLOEXEC) = -1 ENOENT  # load (fails)
```

`TMPDIR` is honored (tested with `TMPDIR=/tmp/opencode/tmpdir`) and does not
change the behavior. `simulation.working_dir` does not affect it either.

## Scope / impact

- **Every** SSP that embeds an FMU as a `.fmu` archive is affected
  (simple_benchmark 3 configs, synthetic_propagation 344 configs).
- The failure is independent of the executor method (`jacobi`, `seidel`,
  `la2`) — it happens during `Simulator.init()` before any executor runs.
- This is **fatal for the normal pipeline**: the `simulate` stage cannot produce
  any result CSV with the v0.3.0 wheel as-is.

## Workaround (verified)

Point the SSP `Component` at a **pre-extracted FMU directory** instead of the
`.fmu` archive. The engine skips the buggy unzip lifecycle when the `source`
path is already an unzipped FMU directory (it uses `fmi4c_loadUnzippedFmu`
directly).

```bash
mkdir -p ssp/resources/mono_fmu
unzip -o ssp/resources/MonolithicReference.fmu -d ssp/resources/mono_fmu
# edit SystemStructure.ssd:
#   source="resources/MonolithicReference.fmu"  ->  source="resources/mono_fmu"
```

Verified working for:
- `monolithic_reference` (jacobi) — result CSV produced, solver runs.
- `chain_n001_s20260529/all_separated` (la2) — result CSV produced.
- `chain_n013_s20261034/all_separated` (la2) — `La2 stack: 13 SCCs, outer`
  logged, correct per-path values 1..13, `doStepCalls` increments.

## Suggested fix (upstream)

In the FMU unzip lifecycle, the temporary extraction directory must not be
removed between `unzipFmu`/`unzip_to_temp_dir` and `fmi4c_loadUnzippedFmu`.
Either keep the temp directory alive until the FMU handle is destroyed, or
move/rename it to a persistent location before `chmod +x` and `dlopen`. The
v0.2.11 wheel's behavior is the reference implementation.

## Repository tracking

- This report: `docs/bug-report-ssp4sim-v030-fmu-extraction.md`
- Historic FMU-extraction blocker (fixed in v0.2.11):
  `.dynamic-harness/findings.md` (claim 38 / A10),
  `.dynamic-harness/simulate_smoke_result.md`,
  `docs/roadmap/study2-maturity.md` §6.5.