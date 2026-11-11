---
id: INFO-016
type: info
title: Code structure
summary: Repository layout and the module dependency graph
date: 2026-11-11
status: current
---

# Code structure

Repository layout and module dependency direction; responsibilities and interfaces are owned by the component view.

- **Top level:** `lib/` core engine, `public/` entry points, `tests/`, `3rdParty/` vendored dependencies, `containers/` build containers.
- **Module dependency graph:** handler FMU loading feeds the analysis graph, which feeds the execution graph; model, execution, and signal all depend on utils.

```
handler/ (FMU load) --> analysis graph --> execution graph
                                          |
model/  execution/  signal/  <------------+
   (FMU wrap) (strategy) (recording)      |--> utils/ (shared)
```

- **Entry points:** CLI `public/ssp4sim_app/`, Python `public/python_api/`, public headers `lib/public_include/`; build-target mapping is the build environment (`INFO-015`).
- **Module areas:** responsibilities and public interfaces are architected in the component view (`INFO-012`); this leaf holds layout and dependency direction only.
