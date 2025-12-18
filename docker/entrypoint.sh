#!/bin/bash
set -e

# Helper function for consistent logging with timestamps
log() {
    echo "[$(date +'%Y-%m-%d %H:%M:%S')] [ENTRYPOINT] $*"
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

        log "Executing: $BINARY_PATH $@"
        exec "$BINARY_PATH" "$@"
        ;;

    init)
        shift
        log "Mode selected: INIT"

        # Validation
        if [ ! -f "$BINARY_PATH" ]; then
            log "ERROR: Binary not found at $BINARY_PATH"
            exit 1
        fi

        log "Executing: $BINARY_PATH $@"
        exec "$BINARY_PATH" "$@"
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
        exec "$SVDIR_PATH" -P /etc/service
        ;;
esac