# Improvement Backlog Overview

subfolders:
candidates - not selected for development
selected - items to be incorporated
done - historical tracking of items



Use this template as the landing area for accepted continuous-improvement candidates.

Generated from improvement workflow:

```text
intake -> broad read-only discovery -> architecture/requirement pressure analysis -> backlog candidates -> final report
```

Each candidate is proposed. None is approved for implementation until it has a scoped task contract.

## Usage

Place the overview at the repository's chosen evolution or backlog location, such as:

```text
product-breakdown/06-evolution/backlog/improvement-backlog.md
```

Put each candidate in its own file beside the overview or in a local `candidates/` directory. Move completed implementations to a local `completed/` directory when the repository uses one.

## Individual Candidates

| File | ID | Theme | Status | Priority | Blast radius |
| --- | --- | --- | --- | --- | --- |
| `candidates/IMP-001.md` | IMP-001 | Deduplicate profiling content between PBS and docs | Proposed | High | 2 files (PBS 05-operation/profiling.md, docs/profiling.md); low risk, text removal only |
| `candidates/IMP-002.md` | IMP-002 | Deduplicate configuration schema tables between PBS and docs | Proposed | High | 2 files (PBS 03-implementation/configuration.md, docs/configuration.md); medium risk, schema tables duplicated |
| `candidates/IMP-003.md` | IMP-003 | Deduplicate contributing/development workflow between PBS and docs | Proposed | Medium | 2 files (PBS 06-evolution/contributing.md, docs/development.md); low risk |
| `candidates/IMP-004.md` | IMP-004 | Tighten release-pipeline PBS boundary to design-only | Proposed | Low | 2 files (PBS 05-operation/release-pipeline.md, docs/linux_binary_distribution.md); low risk |
| `candidates/IMP-005.md` | IMP-005 | Deduplicate generated-data policy between docs and PBS | Proposed | Low | 2 files (docs/development.md, PBS 03-implementation/dependency-policy.md); low risk |
| `candidates/IMP-006.md` | IMP-006 | Reduce docs/overview.md to pure landing page with PBS links | Proposed | Low | 1 file (docs/overview.md); low risk |
| `candidates/IMP-007.md` | IMP-007 | Rename jacobi_serial to Gauss-Seidel + hybrid executor design sketch | Proposed | Medium | Rename: executor_builder.cpp, docs/configuration.md, module docs (moderate). Design sketch: documentation only (low) |
| `candidates/IMP-008.md` | IMP-008 | Newton iteration executor feasibility assessment for algebraic-loop resolution | Proposed | Low | Design sketch only; no implementation (low). If later implemented: large (new solver, interface changes) |
| `candidates/IMP-009.md` | IMP-009 | SignalStorage pointer indirection — close risk as non-bottleneck | Closed | N/A | None — analysis only |

## Summary

| ID | Theme | Priority | Prerequisite | Blast radius |
| --- | --- | --- | --- | --- |
| IMP-001 | Deduplicate profiling content between PBS and docs | High | None | 2 files, text removal only |
| IMP-002 | Deduplicate configuration schema tables between PBS and docs | High | None | 2 files, section removal in PBS |
| IMP-003 | Deduplicate contributing/development workflow between PBS and docs | Medium | None | 2 files, minor text changes |
| IMP-004 | Tighten release-pipeline PBS boundary to design-only | Low | None | 2 files, minor text changes |
| IMP-005 | Deduplicate generated-data policy between docs and PBS | Low | None | 2 files, minor text changes |
| IMP-006 | Reduce docs/overview.md to pure landing page with PBS links | Low | None | 1 file, minor text changes |
| IMP-007 | Rename jacobi_serial to Gauss-Seidel + hybrid executor design sketch | Medium | None | Rename: configuration + docs (moderate). Design sketch: docs only (low) |
| IMP-008 | Newton iteration executor feasibility assessment | Low | None | Design sketch only (low). Implementation deferred |
| IMP-009 | SignalStorage pointer indirection — close risk | N/A | None | None — analysis only |

## Cross-Cutting Constraints

1. **Layer ownership**: `product-breakdown/` holds "what the product is, who it is for, scope, stable decisions." `docs/` holds "runnable guidance: usage, install, build, development workflow, examples, profiling commands."
2. **No duplication**: If a doc needs product context, it must link to `product-breakdown/` rather than copying text. If a PBS file needs runnable commands, it must link to `docs/` rather than reproducing them.
3. **Traceability preservation**: Every PBS file's Sources field that references a `docs/` file is correct and must be preserved. The deduplication removes content but keeps the cross-reference.
4. **Stay out of feature diffs**: None of these candidates should be bundled into unrelated feature or bug-fix work. Each candidate is a standalone documentation-hygiene task.
5. **`AGENTS.md` and `readme.md`**: These files reference both directories and may need link updates if files referenced in their link lists change. Any such changes must be scoped within the candidate's task contract, not added as incidental edits to other work.