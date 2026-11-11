---
id: INFO-025
type: info
title: schema_extensions module
summary: Extension types over FMI 2.0 and SSP 1.0 schema elements parsed from XML
date: 2026-11-11
status: current
---

# schema_extensions module

Data representations of FMI 2.0 and SSP 1.0 schema elements used during model-description parsing.

- **Role:** thin data-layer module — extension types over the parsed XML schema elements; mostly enum definitions and JSON structure representations.
- **Types:** `FMI2_Enums_Ext`, `FMI2_modelDescription_Ext`, `SSP_Ext`, `SSP1_SystemStructureDescription_Ext`, `SSP1_SystemStructureParameter_Ext` (SSV/SSM).
- **Extends vendored types:** extensions augment the base types from the vendored ssp4cpp library.
- **Include boundary:** `lib/include/schema_extensions/`, 11 files covering the extension types and their serialization.
- **Dependencies:** external ssp4cpp only; no internal module dependencies.
- **Documentation:** `extensions.md` records the extension rationale.
