#!/usr/bin/env bash
# Thin sugar over the canonical CTest surface for the standalone per-test
# binaries. CTest (`ctest --test-dir build`) remains the authoritative runner;
# this script builds and runs one or more unit binaries directly.
#
# Usage:
#   tests/run_unit.sh                       # run every standalone unit binary
#                                           #   found in the build tree
#   tests/run_unit.sh --tier T1             # T1 = kernel/units, T2 = parser,
#                                           #     T3 = analysis/graph
#   tests/run_unit.sh --filter <binary>     # exactly one unit binary
#   tests/run_unit.sh -- <catch2 args>      # pass-through, e.g. -- [RingBuffer]
#
# Set BUILD_DIR to point at a non-default build tree.
#
# The default run set is derived from the build tree (whatever test_* binaries
# exist under build/tests/lib), so a new test_<name>.cpp is run here as soon as
# it is built without editing this file. Tier buckets below are the curated
# Phase 0 classification; a built binary that is in no tier triggers a warning
# instead of being silently skipped.

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$REPO_ROOT/build}"
UNIT_DIR="$BUILD_DIR/tests/lib"

# Curated Phase 0 buckets. Keep in sync with the tier matrix in tests/README.md.
T1_BINS=(test_mpsc_event_queue test_node test_node_iterator test_ring_buffer test_config test_thread_pool)
T2_BINS=(test_parameter_binding test_schema_extensions test_signal_storage test_ssp_elements test_start_value)
T3_BINS=(test_graph_builder test_ssp_node test_tree_builder test_parameter_binding_integration)

TIER=""
FILTER=""
EXTRA=()

while [[ $# -gt 0 ]]; do
  case "$1" in
    --tier)
      TIER="$2"
      shift 2
      ;;
    --filter)
      FILTER="$2"
      shift 2
      ;;
    --)
      shift
      EXTRA+=("$@")
      break
      ;;
    -h|--help)
      grep '^#' "$0" | sed 's/^# \{0,1\}//'
      exit 0
      ;;
    *)
      echo "run_unit.sh: unknown argument '$1'" >&2
      exit 2
      ;;
  esac
done

TESTS=()
if [[ -n "$FILTER" ]]; then
  TESTS=("$FILTER")
elif [[ -n "$TIER" ]]; then
  case "$TIER" in
    T1) TESTS=("${T1_BINS[@]}") ;;
    T2) TESTS=("${T2_BINS[@]}") ;;
    T3) TESTS=("${T3_BINS[@]}") ;;
    *)
      echo "run_unit.sh: unknown tier '$TIER' (expected T1, T2 or T3)" >&2
      exit 2
      ;;
  esac
else
  mapfile -t TESTS < <(find "$UNIT_DIR" -maxdepth 1 -type f -name 'test_*' ! -name 'ssp4sim_tests' ! -name '*.cmake' -printf '%f\n' 2>/dev/null | sort)
fi

# Tier-drift guard: a built unit binary not covered by any tier should be new
# and unbucketed, not silently forgotten.
if [[ -n "$TIER" ]]; then
  mapfile -t BUILT < <(find "$UNIT_DIR" -maxdepth 1 -type f -name 'test_*' ! -name 'ssp4sim_tests' ! -name '*.cmake' -printf '%f\n' 2>/dev/null | sort)
  for b in "${BUILT[@]}"; do
    if ! printf '%s\n' "${T1_BINS[@]}" "${T2_BINS[@]}" "${T3_BINS[@]}" | grep -qx "$b"; then
      echo "run_unit.sh: note: built unit binary '$b' is not bucketed in any tier." >&2
      echo "run_unit.sh: add it to the tier lists in tests/run_unit.sh; the default run already includes it." >&2
    fi
  done
fi

cmake --build "$BUILD_DIR" --target "${TESTS[@]}"

for t in "${TESTS[@]}"; do
  echo "=== $t ==="
  "$UNIT_DIR/$t" "${EXTRA[@]:-}"
done