#!/usr/bin/env bash

set -euo pipefail

# Run a non-interactive command in the project container.
# This is intended for automation such as CI, where we want the same build
# environment as the local interactive shell helper without TTY-specific flags.

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"
IMAGE_NAME="${SSP4SIM_CONTAINER_IMAGE:-ssp4sim-ubuntu22-gcc13}"
CONTAINER_HOME="/tmp/ssp4sim-home"

VOLUME_ARGS=()
ENV_ARGS=()
USERNS_ARGS=()
CONTAINER_COMMAND=()

if command -v podman >/dev/null 2>&1; then
    CONTAINER_RUNTIME="podman"
    USERNS_ARGS=(--userns keep-id)
elif command -v docker >/dev/null 2>&1; then
    CONTAINER_RUNTIME="docker"
else
    echo "error: neither podman nor docker was found in PATH" >&2
    exit 1
fi

# Accept optional `--env NAME` or `--env NAME=value` arguments before the
# command. `--env NAME` forwards the current host value if it is set.
while (($# > 0)); do
    case "$1" in
        --env)
            if (($# < 2)); then
                echo "error: --env requires a variable name or NAME=value" >&2
                exit 1
            fi
            if [[ "$2" == *=* ]]; then
                ENV_ARGS+=(--env "$2")
            elif [[ -n "${!2:-}" ]]; then
                ENV_ARGS+=(--env "$2=${!2}")
            fi
            shift 2
            ;;
        --)
            shift
            break
            ;;
        *)
            break
            ;;
    esac
done

if (($# == 0)); then
    echo "error: no command provided" >&2
    echo "usage: $0 [--env NAME|NAME=value ...] -- <command> [args...]" >&2
    exit 1
fi

CONTAINER_COMMAND=("$@")

VOLUME_ARGS+=(--volume "${REPO_ROOT}:/work")

if [[ -d /etc/ssl/certs ]]; then
    VOLUME_ARGS+=(--volume /etc/ssl/certs:/etc/ssl/certs:ro)
fi

# Keep proxy settings when present so network access inside the container
# behaves the same as on the host runner.
for proxy_var in HTTP_PROXY HTTPS_PROXY NO_PROXY http_proxy https_proxy no_proxy; do
    if [[ -n "${!proxy_var:-}" ]]; then
        ENV_ARGS+=(--env "${proxy_var}=${!proxy_var}")
    fi
done

exec "${CONTAINER_RUNTIME}" run \
    --rm \
    --user "$(id -u):$(id -g)" \
    "${USERNS_ARGS[@]}" \
    --env HOME="${CONTAINER_HOME}" \
    --env CC=/usr/bin/gcc-13 \
    --env CXX=/usr/bin/g++-13 \
    --env AR=/usr/bin/gcc-ar-13 \
    --env RANLIB=/usr/bin/gcc-ranlib-13 \
    --env NM=/usr/bin/gcc-nm-13 \
    --workdir /work \
    "${ENV_ARGS[@]}" \
    "${VOLUME_ARGS[@]}" \
    "${IMAGE_NAME}" \
    "${CONTAINER_COMMAND[@]}"
