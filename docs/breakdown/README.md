# SSP4SIM Breakdown

SSP4SIM is a C++23 simulation engine that executes SSP 1.0 archives containing
FMI 2.0 co-simulation FMUs, with Gauss-Jacobi and Gauss-Seidel strategies,
local database recording, tiered logging, and CLI, C, and Python APIs. This
tree is the source of truth for what SSP4SIM is, promises, and is verified
against; `docs/` holds runnable guidance, and this tree decides.

## Method

One layer question per folder. Enter a layer at its `__INDEX__.md`, then open the
leaf that owns the concern. One leaf answers one question.

Shape is flat: `<layer>/<topic>/<leaf>.md`; a topic folder exists only for three
or more leaves, and one or two stay flat beside them. Growth order:
`guidelines/node-shape.md` § Node Shape.

One `__INDEX__.md` per content folder (exceptions: `guidelines/index-rules.md` § Index); its
generated `## Contents` lists the direct children. Reading order is
`[layers].order` in `pb.toml`.

Boundary: the breakdown is the source of truth; design detail defaults to the
lower layer, and ownership decides where a fact lives. Each layer answers one
question; the method's layer guide owns which layer owns which fact, and each
layer's own index states its question. Review routes findings back to the owning
layer. Current state lives in the leaves; history in the decision archive,
reached through `pb archive` (or loose `decisions/` when no archive); open work
in `06-evolution/`.

Excludes: runnable commands, install/build walkthroughs, and concrete tool
usage live in `docs/` (`usage.md`, `configuration.md`, `development.md`); the
breakdown states policy and decisions, never runnable detail.
