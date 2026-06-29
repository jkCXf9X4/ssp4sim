# Module: schema_extensions
<!-- Layer: 03-implementation -->
<!-- Stable ID: IMP-MOD-SCHEMA-001 -->

## Purpose

Provides extension types for FMI 2.0 and SSP 1.0 schema elements parsed from XML. These are data representations of the standard schema types used during model description parsing.

## Key Components

- `FMI2_Enums_Ext`: FMI 2.0 enum type definitions.
- `FMI2_modelDescription_Ext`: FMI 2.0 model description extension types.
- `SSP_Ext`: SSP archive structure extension types.
- `SSP1_SystemStructureDescription_Ext`: SSD XML extension types.
- `SSP1_SystemStructureParameter_Ext`: SSV/SSM parameter extension types.

## Include Boundary

- Path: `lib/include/schema_extensions/`
- 11 files covering all extension types and their serialization.

## Dependencies

- External: ssp4cpp (vendored, for base SSP types)
- No internal module dependencies

## Notable Patterns

- Thin data-layer module — mostly enum definitions and JSON structure representations.
- Extensions augment the base types from the vendored ssp4cpp library.
- The `extensions.md` file documents the extension rationale.

## Traceability

- Backward: Architecture component view (schema_extensions/ module area).
- Sources: `lib/include/schema_extensions/`, `lib/include/schema_extensions/extensions.md`.