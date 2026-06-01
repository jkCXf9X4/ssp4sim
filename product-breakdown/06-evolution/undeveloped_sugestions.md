
I wanted multiple concurrent simulations to execute against the same local database
BUT for SQLite, the writer constraint is effectively per database file, not per table.

  With WAL mode:

  - Multiple readers can read the same .sqlite file while a writer is active.
  - Only one connection can hold the write lock for that database file at a time.
  - That write lock blocks other writers even if they target different tables.
  - So two executions writing different generated data tables plus ssp4sim_metadata in the same .sqlite file still serialize.

  So the busy timeout improvement helps multiple executions share one result database by waiting instead of failing, but it does not make table-level parallel writes possible.

  If true write parallelism is needed, the practical options are:

  - one SQLite file per execution, then merge/query across files later;
  - one SQLite file per partition/model if the viewer can handle multiple attached databases;
  - use DuckDB or another backend for high-throughput multi-table ingestion, depending on the reader/writer requirements.

In light of this: investigate an improvement regarding individual storage of simulation runs in per sim local db

Some guidelines:
Utilize an UUID that is session based to create a unique file for each simulation
start the filename with epoch seconds to ensure folder order
this also means that the internal table names can be simplified

Make sure to update the use-cases and document the limitations

---
**Formalized as:** [IMP-032](backlog/candidates/IMP-032.md) — Per-Simulation SQLite Database Files for Concurrent Write Parallelism