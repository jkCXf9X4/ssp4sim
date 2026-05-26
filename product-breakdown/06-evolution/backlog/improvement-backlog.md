# Improvement Backlog Overview

Generated from improvement workflow: broad read-only discovery of existing project documentation → product-breakdown layer mapping → backlog candidates.

Each candidate is proposed. None is approved for implementation until it has a scoped task contract.

## Individual Candidates

| File | ID | Theme | Status | Priority | Blast radius |
| --- | --- | --- | --- | --- | --- |
| `candidates/IMP-001.md` | IMP-001 | Document-to-layer mapping overview | Proposed | Medium | Repository-wide (documentation only) |
| `candidates/IMP-002.md` | IMP-002 | Populate intent layer (00-intent) | Proposed | High | New artifacts only; no existing doc edits |
| `candidates/IMP-003.md` | IMP-003 | Populate product layer (01-product) | Proposed | High | New artifacts only; no existing doc edits |
| `candidates/IMP-004.md` | IMP-004 | Requirements-to-test traceability | Proposed | Medium | 04-verification layer; test docs unaffected |
| `candidates/IMP-005.md` | IMP-005 | Documentation layer consolidation | Proposed | Low | Existing `docs/` files; coordinated rename/split |
| `candidates/IMP-006.md` | IMP-006 | Populate architecture layer (02-architecture) | Proposed | High | New artifacts under `product-breakdown/02-architecture/` |
| `candidates/IMP-007.md` | IMP-007 | Populate implementation layer (03-implementation) | Proposed | High | New artifacts under `product-breakdown/03-implementation/` |
| `candidates/IMP-008.md` | IMP-008 | Populate full verification layer (04-verification) | Proposed | Medium | New artifacts under `product-breakdown/04-verification/` |
| `candidates/IMP-009.md` | IMP-009 | Populate operation layer (05-operation) | Proposed | Low | New artifacts under `product-breakdown/05-operation/` |

## Summary

| ID | Theme | Priority | Prerequisite | Blast radius |
| --- | --- | --- | --- | --- |
| IMP-001 | Document-to-layer mapping overview | Medium | None (discovery complete) | Repository-wide (documentation only) |
| IMP-002 | Populate intent layer (00-intent) | High | IMP-001 (layer map known) | New artifacts under `product-breakdown/00-intent/` |
| IMP-003 | Populate product layer (01-product) | High | IMP-002 (intent anchors needed first) | New artifacts under `product-breakdown/01-product/` |
| IMP-004 | Requirements-to-test traceability | Medium | IMP-003 (formal requirements needed) | 04-verification layer; traceability matrix |
| IMP-005 | Documentation layer consolidation | Low | IMP-001, IMP-002, IMP-003 (target layer content must exist) | Existing `docs/` files; cross-references |
| IMP-006 | Populate architecture layer (02-architecture) | High | IMP-003 (product capabilities needed as architecture input) | New artifacts under `product-breakdown/02-architecture/` |
| IMP-007 | Populate implementation layer (03-implementation) | High | IMP-006 (architecture anchors needed first) | New artifacts under `product-breakdown/03-implementation/` |
| IMP-008 | Populate full verification layer (04-verification) | Medium | IMP-003 (formal requirements needed for traceability) | New artifacts under `product-breakdown/04-verification/` |
| IMP-009 | Populate operation layer (05-operation) | Low | IMP-006, IMP-007 (architecture and implementation docs needed for deployment topology) | New artifacts under `product-breakdown/05-operation/` |

## Cross-Cutting Constraints

- No existing project documentation (`readme.md`, `docs/`, `lib/`, `tests/`, `resources/`) may be edited outside a coordinated consolidation task.
- The product-breakdown tree is the canonical location for layer-separated artifacts. Existing docs continue to serve their current audiences until explicitly superseded.
- All candidates respect KM-005 (documentation layer separation with backward traceability). No candidate proposes adding forward traces from higher layers to implementation details.
- The `todo.md` file is empty and currently provides no value; it could be removed or repopulated as part of evolution-layer work.
- IMP-004 and IMP-008 both own `acceptance-criteria.md` in the 04-verification layer. IMP-004 writes the initial version; IMP-008 extends with verification-specific criteria (coverage, CI gates, regression policy). Per maintainer decision.

## Related Lessons

- **KM-005**: Preserve documentation layer separation and backward traceability. These candidates systematically apply that rule by identifying which existing docs mix layers and proposing how layer-separated artifacts would resolve the mixing.
- **KM-008**: Avoid orphaned information nodes. When layer-separated artifacts are created, each must include backward trace links to the artifact or decision it satisfies.