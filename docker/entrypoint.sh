#!/bin/bash
set -e

MODE="$1"

case "$MODE" in
    client)
        shift
        # UPDATED PATH: /usr/local/bin/machinepay
        exec /usr/local/bin/machinepay "$@"
        ;;
    init)
        shift
        # UPDATED PATH: /usr/local/bin/machinepay
        exec /usr/local/bin/machinepay "$@"
        ;;
    *)
        # On Ubuntu/Debian, runsvdir is located in /usr/bin
        exec /usr/bin/runsvdir /etc/service
        ;;
esac