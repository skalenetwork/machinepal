#!/bin/bash
set -e

# CONSTANTS
BINARY_PATH="/usr/local/bin/machinepal"
DATA_DIR="/machinepal"


DATA_DIR="/machinepal"

mountpoint -q "$DATA_DIR" || {
    echo "Error: $DATA_DIR directory of machinepal docker container  must be docker mapped to an external volume that contains machinepal config." >&2
    echo "This can be done using e.g. -v \$(pwd):$DATA_DIR" >&2
    exit 1
}

# ----------------------------------------------------------------------
# 0. DIAGNOSTICS & LOGGING
# ----------------------------------------------------------------------

# Helper function for consistent logging with timestamps
log() {
    printf '[%s] [ENTRYPOINT] %s\n' "$(date +'%Y-%m-%d %H:%M:%S')" "$*" >&2
}

# Always print earliest possible diagnostics to stderr for debugging boot issues
printf '[ENTRYPOINT] argv: %s | pwd=%s | uid=%s gid=%s | whoami=%s\n' \
  "$*" "$(pwd)" "$(id -u)" "$(id -g)" "$(whoami)" >&2

# Enable debug tracing if requested via env var
if [ "${ENTRYPOINT_DEBUG:-0}" = "1" ]; then
  set -x
fi

# ----------------------------------------------------------------------
# 1. RESCUE MODE / SHELL OVERRIDE
# ----------------------------------------------------------------------
# If the user asks for a shell explicitly, bypass the app logic.
if [ "$1" = "/bin/bash" ] || [ "$1" = "/bin/sh" ]; then
    log "Shell requested, executing command directly..."
    exec "$@"
fi

# ----------------------------------------------------------------------
# 2. SANITY CHECKS
# ----------------------------------------------------------------------

# Check if the binary exists
if [ ! -f "$BINARY_PATH" ]; then
    log "CRITICAL ERROR: Binary not found at $BINARY_PATH"
    exit 1
fi

# Check if data directory exists (volume mount check)
if [ ! -d "$DATA_DIR" ]; then
    log "ERROR: $DATA_DIR directory missing."
    log "Did you forget to mount the volume? (-v \"\$(pwd):/machinepal\")"
    # We sleep briefly to prevent a restart-loop storm if managed by systemd/docker restart policy
    sleep 5
    exit 1
fi

# ----------------------------------------------------------------------
# 3. EXECUTION
# ----------------------------------------------------------------------

log "Starting MachinePal..."
log "Running as user: $(whoami)"
log "Command: $BINARY_PATH $*"

# Use stdbuf if available to force line buffering (better docker logs)
if command -v stdbuf >/dev/null 2>&1; then
    # -oL = stdout line buffered, -eL = stderr line buffered
    exec stdbuf -oL -eL "$BINARY_PATH" "$@"
else
    # Fallback if coreutils is missing
    exec "$BINARY_PATH" "$@"
fi