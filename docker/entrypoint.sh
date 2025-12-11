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
        exec /usr/sbin/runsvdir /etc/service
        ;;
esac