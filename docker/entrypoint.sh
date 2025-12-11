#!/bin/bash
set -e

# Capture the mode (first argument)
MODE="$1"

case "$MODE" in
    client)
        # Remove 'client' from arguments list
        shift
        # Run binary with the REST of the arguments
        exec /machinepay/machinepay "$@"
        ;;
    init)
        # Remove 'init' from arguments list
        shift
        # Run binary with the REST of the arguments
        exec /machinepay/machinepay "$@"
        ;;
    *)
        # If argument is empty (or unknown), run the service manager
        exec /usr/sbin/runsvdir /etc/service
        ;;
esac