#!/bin/bash
set -e

# Always print earliest possible diagnostics (stderr)
printf '[ENTRYPOINT] argv: %s | pwd=%s | uid=%s gid=%s | whoami=%s\n' \
  "$*" "$(pwd)" "$(id -u)" "$(id -g)" "$(whoami)" >&2

# Enable debug tracing when requested
if [ "${ENTRYPOINT_DEBUG:-0}" = "1" ]; then
  set -x
fi

# Unmissable startup line (stderr)
echo "[ENTRYPOINT] entrypoint.sh is running (pid=$$ user=$(id -u):$(id -g) whoami=$(whoami))" >&2

# Helper function for consistent logging with timestamps
log() {
    # stderr is typically unbuffered in container logging paths
    printf '[%s] [ENTRYPOINT] %s\n' "$(date +'%Y-%m-%d %H:%M:%S')" "$*" >&2
}

# Run a command with line-buffered stdout/stderr when possible (coreutils: stdbuf)
run() {
    if command -v stdbuf >/dev/null 2>&1; then
        exec stdbuf -oL -eL "$@"
    else
        exec "$@"
    fi
}

MODE="$1"
BINARY_PATH="/usr/local/bin/machinepal"

# Print basic info
log "Starting entrypoint script..."
log "Running as user: $(whoami)"

case "$MODE" in
    client)
        shift
        log "Mode selected: CLIENT"

        # Validation: Ensure the binary exists before trying to run it
        if [ ! -f "$BINARY_PATH" ]; then
            log "ERROR: Binary not found at $BINARY_PATH"
            exit 1
        fi

        log "Executing: $BINARY_PATH $*"
        run "$BINARY_PATH" "$@"
        ;;

    init)
        shift
        log "Mode selected: INIT"

        # Validation
        if [ ! -f "$BINARY_PATH" ]; then
            log "ERROR: Binary not found at $BINARY_PATH"
            exit 1
        fi

        log "Executing: $BINARY_PATH $*"
        run "$BINARY_PATH" "$@"
        ;;

    *)
        log "Mode selected: SERVICE MANAGER (Default)"

        # Check if runsvdir exists (Ubuntu path)
        if [ -x /usr/bin/runsvdir ]; then
             SVDIR_PATH="/usr/bin/runsvdir"
        elif [ -x /usr/sbin/runsvdir ]; then
             SVDIR_PATH="/usr/sbin/runsvdir"
        else
            log "ERROR: runsvdir not found! Is 'runit' installed?"
            exit 1
        fi

        log "Starting runit service supervisor..."
        run "$SVDIR_PATH" -P /etc/service
        ;;
esac