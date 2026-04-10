#!/usr/bin/env bash

set -euo pipefail

# Build the project container image used for local reproducible builds.
# The image mirrors the Ubuntu 22.04 + GCC 13 environment used in CI.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
CONTAINER_DIR="${REPO_ROOT}/containers/ubuntu22-gcc13"
CONTAINERFILE="${CONTAINER_DIR}/Containerfile"
IMAGE_NAME="${SSP4SIM_CONTAINER_IMAGE:-ssp4sim-ubuntu22-gcc13}"

# Prefer Podman when available because it works well rootless. Fall back to
# Docker so developers can use whichever runtime they already have installed.
if command -v podman >/dev/null 2>&1; then
    CONTAINER_RUNTIME="podman"
elif command -v docker >/dev/null 2>&1; then
    CONTAINER_RUNTIME="docker"
else
    echo "error: neither podman nor docker was found in PATH" >&2
    exit 1
fi

# The build context is the repository root so the Containerfile can copy or
# refer to files from the project if needed in the future.
exec "${CONTAINER_RUNTIME}" build \
    --file "${CONTAINERFILE}" \
    --tag "${IMAGE_NAME}" \
    "${REPO_ROOT}"
