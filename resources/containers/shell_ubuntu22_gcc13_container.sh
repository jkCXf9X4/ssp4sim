#!/usr/bin/env bash

set -euo pipefail

# Open an interactive shell in the project container.
# The repository is bind-mounted at /work so builds and edits happen directly
# against the host checkout.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
IMAGE_NAME="${SSP4SIM_CONTAINER_IMAGE:-ssp4sim-ubuntu22-gcc13}"
CONTAINER_NAME="${SSP4SIM_CONTAINER_NAME:-ssp4sim-ubuntu22-gcc13-shell}"
CONTAINER_HOME="/tmp/ssp4sim-home"

# Build the volume list incrementally to keep the final `run` command readable.
VOLUME_ARGS=()

# Prefer Podman when available because it supports rootless containers well.
# With Podman, `--userns keep-id` preserves access to bind-mounted files in the
# host checkout. Docker does not use the same flag.
if command -v podman >/dev/null 2>&1 && podman info >/dev/null 2>&1; then
    CONTAINER_RUNTIME="podman"
    USERNS_ARGS=(--userns keep-id)
elif command -v docker >/dev/null 2>&1; then
    CONTAINER_RUNTIME="docker"
    USERNS_ARGS=()
else
    echo "error: neither podman nor docker was found in PATH" >&2
    exit 1
fi

# Mount the repository into the working directory used inside the container.
VOLUME_ARGS+=(--volume "${REPO_ROOT}:/work")

# Reuse the host CA store so TLS-inspecting corporate proxies do not break
# vcpkg downloads inside the container.
if [[ -d /etc/ssl/certs ]]; then
    VOLUME_ARGS+=(--volume /etc/ssl/certs:/etc/ssl/certs:ro)
fi

# Run as the current host user so files created in /work keep sensible
# ownership on the host checkout.
exec "${CONTAINER_RUNTIME}" run \
    --rm \
    --interactive \
    --tty \
    --name "${CONTAINER_NAME}" \
    --user "$(id -u):$(id -g)" \
    "${USERNS_ARGS[@]}" \
    --env HOME="${CONTAINER_HOME}" \
    "${VOLUME_ARGS[@]}" \
    "${IMAGE_NAME}" \
    /bin/bash
