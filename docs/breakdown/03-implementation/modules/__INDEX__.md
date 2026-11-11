---
title: Modules
summary: Concrete module-by-module implementation facts
---

# Modules

## Contents

<!-- pb:index:start -->
<!-- GENERATED FILE — do not edit. Regenerate with `pb registers`. -->
- **INFO-020** [Data access and scheduling — design space](data_access_and_scheduling_design.md) — Candidate designs A–H for deterministic data access, the current design's known defects, and the non-binding lean
- **INFO-021** [Data access and scheduling — problem space](data_access_and_scheduling_problem_space.md) — The problem, vocabulary, and axes the design space must satisfy
- **INFO-022** [Data access and scheduling — access-policy architecture](data_access_policy_architecture.md) — Candidate I — data access as a pluggable strategy beside the executor; candidacy, mitigations, open questions
- **INFO-023** [execution module](execution.md) — Executor strategies that advance simulation time and coordinate FMU stepping
- **INFO-024** [signal module](signal.md) — In-memory simulation data layout and the recorder/export path
- **INFO-025** [schema_extensions module](schema_extensions.md) — Extension types over FMI 2.0 and SSP 1.0 schema elements parsed from XML
- **INFO-026** [utils module](utils.md) — Shared low-level helpers used by every other module
- **INFO-040** [Data access and scheduling — read-target resolver](data_access_read_target_resolver.md) — The graph-aware narrowing of candidate I, the sub-executor question, and policy-side open questions
- **INFO-041** [Data access and scheduling — read behaviors](data_access_use_cases.md) — The fourteen design-side read behaviors the data-access design must express
- **INFO-042** [Data access and scheduling — known difficulties](data_access_difficulties.md) — The eighteen known difficulties (P1–P18) in the data-access problem space
- **INFO-043** [Data access and scheduling — open questions](data_access_open_questions.md) — The unresolved problem-side questions the data-access design space must answer
<!-- pb:index:end -->
