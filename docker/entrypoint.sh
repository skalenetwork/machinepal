#!/bin/bash
set -e

MODE="$1"

case "$MODE" in
    client)
        shift
        # UPDATED PATH: /usr/local/bin/machinepal
        exec /usr/local/bin/machinepal "$@"
        ;;
    init)
        shift
        # UPDATED PATH: /usr/local/bin/machinepal
        exec /usr/local/bin/machinepal "$@"
        ;;
    *)
        # On Ubuntu/Debian, runsvdir is located in /usr/bin
        exec /usr/bin/runsvdir /etc/service
        ;;
esac